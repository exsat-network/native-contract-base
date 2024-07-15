#include <blksync.xsat/blksync.xsat.hpp>
#include <poolreg.xsat/poolreg.xsat.hpp>
#include <rescmng.xsat/rescmng.xsat.hpp>
#include <utxomng.xsat/utxomng.xsat.hpp>
#include <bitcoin/core/block_header.hpp>
#include <bitcoin/script/address.hpp>
#include <cmath>
#include "../internal/defines.hpp"

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

// @auth utxomng.xsat
[[eosio::action]]
void block_sync::consensus(const uint64_t height, const name& synchronizer, const uint64_t bucket_id) {
    require_auth(UTXO_MANAGE_CONTRACT);

    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_itr
        = _block_bucket.require_find(bucket_id, "blksync.xsat::consensus: block bucket does not exists");
    _block_bucket.erase(block_bucket_itr);

    // erase passed index
    passed_index_table _passed_index(get_self(), height);
    auto passed_index_itr = _passed_index.begin();
    while (passed_index_itr != _passed_index.end()) {
        passed_index_itr = _passed_index.erase(passed_index_itr);
    }

    // erase block miner
    block_miner_table _block_miner(get_self(), height);
    auto block_miner_itr = _block_miner.begin();
    while (block_miner_itr != _block_miner.end()) {
        block_miner_itr = _block_miner.erase(block_miner_itr);
    }
}

//@auth utxomng.xsat
[[eosio::action]]
void block_sync::delchunks(const uint64_t height, const uint64_t bucket_id) {
    require_auth(UTXO_MANAGE_CONTRACT);

    block_chunk_table _block_chunk = block_chunk_table(get_self(), bucket_id);
    auto block_chunk_itr = _block_chunk.begin();
    while (block_chunk_itr != _block_chunk.end()) {
        block_chunk_itr = _block_chunk.erase(block_chunk_itr);
    }
}

//@auth synchronizer
[[eosio::action]]
void block_sync::initbucket(const name& synchronizer, const uint64_t height, const checksum256& hash,
                            const uint32_t block_size, const uint8_t num_chunks) {
    require_auth(synchronizer);

    check(height > START_HEIGHT, "blksync.xsat::initbucket: height must be greater than 840000");
    check(block_size > BLOCK_HEADER_SIZE, "blksync.xsat::initbucket: block size must be greater than 80");
    check(num_chunks > 0, "blksync.xsat::initbucket: the number of chunks must be greater than 0");

    // check whether it is a synchronizer
    pool::synchronizer_table _synchronizer(POOL_REGISTER_CONTRACT, POOL_REGISTER_CONTRACT.value);
    auto synchronizer_itr
        = _synchronizer.require_find(synchronizer.value, "blksync.xsat::initbucket: not an synchronizer account");

    check(!utxo_manage::check_consensus(height, hash), "blksync.xsat::initbucket: the block has reached consensus");

    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_idx = _block_bucket.get_index<"byblockid"_n>();
    auto block_bucket_itr = block_bucket_idx.find(xsat::utils::compute_block_id(height, hash));

    uint64_t bucket_id = 0;
    if (block_bucket_itr == block_bucket_idx.end()) {
        // slot limit
        check(synchronizer_itr->produced_block_limit == 0
                  || height - synchronizer_itr->latest_produced_block_height <= synchronizer_itr->produced_block_limit,
              "blksync.xsat::initbucket: to become a synchronizer, a block must be produced within 72 hours");
        const auto num_slots = std::distance(_block_bucket.begin(), _block_bucket.end());
        check(num_slots < synchronizer_itr->num_slots,
              "blksync.xsat::initbucket: not enough slots, please buy more slots");

        bucket_id = next_bucket_id();
        _block_bucket.emplace(get_self(), [&](auto& row) {
            row.bucket_id = bucket_id;
            row.height = height;
            row.hash = hash;
            row.size = block_size;
            row.num_chunks = num_chunks;
            row.status = uploading;
        });
    } else {
        check(block_bucket_itr->status == uploading,
              "blksync.xsat::initbucket: cannot init bucket in the current state ["
                  + get_block_status_name(block_bucket_itr->status) + "]");

        bucket_id = block_bucket_itr->bucket_id;
        block_bucket_idx.modify(block_bucket_itr, same_payer, [&](auto& row) {
            row.size = block_size;
            row.num_chunks = num_chunks;
            row.status = uploading;
        });
    }

    // log
    block_sync::bucketlog_action _bucketlog(get_self(), {get_self(), "active"_n});
    _bucketlog.send(bucket_id, synchronizer, height, hash, block_size, num_chunks);
}

//@auth synchronizer
[[eosio::action]]
void block_sync::pushchunk(const name& synchronizer, const uint64_t height, const checksum256& hash,
                           const uint8_t chunk_id, const std::vector<char>& data) {
    require_auth(synchronizer);

    check(!utxo_manage::check_consensus(height, hash), "blksync.xsat::pushchunk: the block has reached consensus");

    // check
    check(data.size() > 0, "blksync.xsat::pushchunk: data size must be greater than 0");

    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_idx = _block_bucket.get_index<"byblockid"_n>();
    auto block_bucket_itr = block_bucket_idx.require_find(xsat::utils::compute_block_id(height, hash),
                                                          "blksync.xsat::pushchunk: [blockbuckets] does not exists");

    auto status = block_bucket_itr->status;
    check(status != verify_merkle && status != verify_parent_hash && status != verify_pass,
          "blksync.xsat::pushchunk: cannot push chunk in the current state [" + get_block_status_name(status) + "]");

    // fee deduction
    resource_management::pay_action pay(RESOURCE_MANAGE_CONTRACT, {get_self(), "active"_n});
    pay.send(height, hash, synchronizer, PUSH_CHUNK, 1);

    uint32_t pre_size = 0;
    // emplace/modify chunk
    block_chunk_table _block_chunk = block_chunk_table(get_self(), block_bucket_itr->bucket_id);
    auto chunk_itr = _block_chunk.find(chunk_id);
    if (chunk_itr != _block_chunk.end()) {
        pre_size = chunk_itr->data.size();
        _block_chunk.modify(chunk_itr, get_self(), [&](auto& row) {
            row.data = data;
        });
    } else {
        _block_chunk.emplace(get_self(), [&](auto& row) {
            row.id = chunk_id;
            row.data = data;
        });
    }

    block_bucket_idx.modify(block_bucket_itr, same_payer, [&](auto& row) {
        if (chunk_itr == _block_chunk.end()) {
            row.uploaded_num_chunks += 1;
        }
        row.uploaded_size = row.uploaded_size + data.size() - pre_size;

        if (row.uploaded_size == row.size && row.uploaded_num_chunks == row.num_chunks) {
            row.status = upload_complete;
        } else {
            row.status = uploading;
        }
    });
}

//@auth synchronizer
[[eosio::action]]
void block_sync::delchunk(const name& synchronizer, const uint64_t height, const checksum256& hash,
                          const uint8_t chunk_id) {
    require_auth(synchronizer);

    // check
    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_idx = _block_bucket.get_index<"byblockid"_n>();
    auto block_bucket_itr = block_bucket_idx.require_find(xsat::utils::compute_block_id(height, hash),
                                                          "blksync.xsat::delchunk: [blockbuckets] does not exists");

    auto status = block_bucket_itr->status;
    check(status != verify_merkle && status != verify_parent_hash && status != verify_pass,
          "blksync.xsat::delchunk: cannot delete chunk in the current state [" + get_block_status_name(status) + "]");

    block_chunk_table _block_chunk = block_chunk_table(get_self(), block_bucket_itr->bucket_id);
    auto itr = _block_chunk.require_find(chunk_id, "blksync.xsat::delchunk: chunk_id does not exist");
    auto chunk_size = itr->data.size();
    _block_chunk.erase(itr);

    block_bucket_idx.modify(block_bucket_itr, same_payer, [&](auto& row) {
        row.uploaded_num_chunks -= 1;
        row.uploaded_size -= chunk_size;

        if (row.uploaded_size == row.size && row.uploaded_num_chunks == row.num_chunks) {
            row.status = upload_complete;
        } else {
            row.status = uploading;
        }
    });
}

//@auth synchronizer
[[eosio::action]]
void block_sync::delbucket(const name& synchronizer, const uint64_t height, const checksum256& hash) {
    require_auth(synchronizer);

    // check
    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_idx = _block_bucket.get_index<"byblockid"_n>();
    auto block_bucket_itr = block_bucket_idx.require_find(xsat::utils::compute_block_id(height, hash),
                                                          "blksync.xsat::delbucket: [blockbuckets] does not exists");

    // erase block chunks
    block_chunk_table _block_chunk = block_chunk_table(get_self(), block_bucket_itr->bucket_id);
    auto block_chunk_itr = _block_chunk.begin();
    while (block_chunk_itr != _block_chunk.end()) {
        block_chunk_itr = _block_chunk.erase(block_chunk_itr);
    }

    // erase block bucket
    block_bucket_idx.erase(block_bucket_itr);

    // erase passed index
    passed_index_table _passed_index(get_self(), height);
    auto passed_index_idx = _passed_index.get_index<"bybucketid"_n>();
    auto passed_index_itr = passed_index_idx.find(block_bucket_itr->bucket_id);
    if (passed_index_itr != passed_index_idx.end()) {
        passed_index_idx.erase(passed_index_itr);
    }
}

//@auth synchronizer
[[eosio::action]]
block_sync::verify_block_result block_sync::verify(const name& synchronizer, const uint64_t height,
                                                   const checksum256& hash) {
    require_auth(synchronizer);

    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_idx = _block_bucket.get_index<"byblockid"_n>();
    auto block_bucket_itr = block_bucket_idx.require_find(
        xsat::utils::compute_block_id(height, hash),
        "blksync.xsat::verify: you have not uploaded the block data. please upload it first and then verify it");
    check(block_bucket_itr->in_verifiable(), "blksync.xsat::verify: cannot validate block in the current state ["
                                                 + get_block_status_name(block_bucket_itr->status) + "]");

    if (utxo_manage::check_consensus(height, hash)) {
        return check_fail(block_bucket_idx, block_bucket_itr, "reached_consensus", hash);
    }

    // fee deduction
    resource_management::pay_action pay(RESOURCE_MANAGE_CONTRACT, {get_self(), "active"_n});
    pay.send(height, hash, synchronizer, VERIFY, 1);
    auto status = block_bucket_itr->status;

    auto verify_info = block_bucket_itr->verify_info;
    checksum256 cumulative_work = checksum256();
    name miner;
    vector<string> btc_miners;
    if (status == upload_complete || status == verify_merkle) {
        if (!verify_info.has_value()) {
            verify_info = verify_info_data{};
        }

        auto block_data = read_bucket(block_bucket_itr->bucket_id, get_self(), verify_info->processed_position,
                                      block_bucket_itr->size);
        eosio::datastream<const char*> block_stream(block_data.data(), block_data.size());

        // verify header
        if (verify_info->processed_position == 0) {
            bitcoin::core::block_header block_header;
            block_stream >> block_header;

            auto block_hash = bitcoin::be_checksum256_from_uint(block_header.hash());

            if (block_hash != hash) {
                return check_fail(block_bucket_idx, block_bucket_itr, "hash_mismatch", block_hash);
            }

            if (block_header.target_is_valid() == false) {
                return check_fail(block_bucket_idx, block_bucket_itr, "invalid_target", block_hash);
            }

            verify_info->previous_block_hash = bitcoin::be_checksum256_from_uint(block_header.previous_block_hash);
            verify_info->work = bitcoin::be_checksum256_from_uint(block_header.work());
            // check transactions size
            verify_info->num_transactions = bitcoin::varint::decode(block_stream);
            verify_info->header_merkle = bitcoin::le_checksum256_from_uint(block_header.merkle);
            if (verify_info->num_transactions == 0) {
                return check_fail(block_bucket_idx, block_bucket_itr, "tx_size_limits", block_hash);
            }
        }
        utxo_manage::config_table _config(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
        auto config = _config.get();
        auto num_txs_per_verification = config.num_txs_per_verification;
        // deserialization transaction
        auto pending_transactions = verify_info->num_transactions - verify_info->processed_transactions;
        auto rows = num_txs_per_verification;
        if (rows > pending_transactions) {
            rows = pending_transactions;
        }

        std::vector<bitcoin::core::transaction> transactions;
        transactions.reserve(rows);
        for (auto i = 0; i < rows; i++) {
            bitcoin::core::transaction transaction;
            block_stream >> transaction;
            transactions.emplace_back(std::move(transaction));
        }

        if (!verify_info->has_witness) {
            verify_info->has_witness = std::any_of(transactions.cbegin(), transactions.cend(), [](const auto& trx) {
                return trx.witness.size() > 0;
            });
        }

        // check witness ?
        if (verify_info->processed_position == 0 && transactions.front().inputs.size() > 0) {
            if (!transactions.front().is_coinbase()) {
                return check_fail(block_bucket_idx, block_bucket_itr, "coinbase_missing", hash);
            }
            const auto& cbtrx = transactions.front();
            verify_info->witness_reserve_value = cbtrx.get_witness_reserve_value();
            verify_info->witness_commitment = cbtrx.get_witness_commitment();

            find_miner(cbtrx.outputs, miner, btc_miners);
        }

        auto need_witness_check
            = verify_info->witness_reserve_value.has_value() && verify_info->witness_commitment.has_value();

        // obtain relay merkle
        bitcoin::uint256_t header_merkle = bitcoin::core::generate_header_merkle(transactions);
        bitcoin::uint256_t witness_merkle;
        if (need_witness_check) {
            witness_merkle = bitcoin::core::generate_witness_merkle(transactions);
        }

        // calculate to the same layer
        if (verify_info->num_transactions > num_txs_per_verification && rows != num_txs_per_verification) {
            uint8_t current_layer = static_cast<uint8_t>(std::ceil(std::log2(rows)));
            vector<bitcoin::uint256_t> cur_hashes;
            cur_hashes.reserve(2);
            for (; current_layer < config.num_merkle_layer; current_layer++) {
                cur_hashes = {header_merkle, header_merkle};
                header_merkle = bitcoin::generate_merkle_root(cur_hashes);
                if (need_witness_check) {
                    cur_hashes = {witness_merkle, witness_merkle};
                    witness_merkle = bitcoin::generate_merkle_root(cur_hashes);
                }
            }
        }

        // save sub merkle
        verify_info->relay_header_merkle.emplace_back(bitcoin::le_checksum256_from_uint(header_merkle));
        if (need_witness_check) {
            verify_info->relay_witness_merkle.emplace_back(bitcoin::le_checksum256_from_uint(witness_merkle));
        }

        // save processed position
        verify_info->processed_transactions += rows;
        verify_info->processed_position += block_stream.tellp();

        // check data size
        if (verify_info->num_transactions == verify_info->processed_transactions
            && verify_info->processed_position < block_bucket_itr->size) {
            return check_fail(block_bucket_idx, block_bucket_itr, "data_exceeds", hash);
        }

        if (verify_info->processed_position == block_bucket_itr->size
            && verify_info->num_transactions > verify_info->processed_transactions) {
            return check_fail(block_bucket_idx, block_bucket_itr, "missing_block_data", hash);
        }

        // verify merkle
        if (verify_info->num_transactions == verify_info->processed_transactions
            && verify_info->processed_position == block_bucket_itr->size) {
            // verify header merkle
            auto header_merkle_root = bitcoin::generate_merkle_root(verify_info->relay_header_merkle);
            if (header_merkle_root != bitcoin::le_uint_from_checksum256(verify_info->header_merkle)) {
                return check_fail(block_bucket_idx, block_bucket_itr, "merkle_invalid", hash);
            }

            // verify witness merkle
            if (need_witness_check) {
                auto witness_merkle_root = bitcoin::core::generate_witness_merkle(verify_info->relay_witness_merkle,
                                                                                  *verify_info->witness_reserve_value);
                if (witness_merkle_root != bitcoin::le_uint_from_checksum256(*verify_info->witness_commitment)) {
                    return check_fail(block_bucket_idx, block_bucket_itr, "witness_merkle_invalid", hash);
                }
            } else if (verify_info->has_witness) {
                return check_fail(block_bucket_idx, block_bucket_itr, "witness_merkle_invalid", hash);
            }
            // next action
            status = verify_parent_hash;
        } else {
            status = verify_merkle;
        }
    }

    if (status == verify_parent_hash) {
        // check parent
        auto parent_cumulative_work = utxo_manage::get_cumulative_work(height - 1, verify_info->previous_block_hash);
        check(parent_cumulative_work.has_value(), "blksync.xsat::verify: parent block hash did not reach consensus");

        cumulative_work
            = bitcoin::be_checksum256_from_uint(bitcoin::be_uint_from_checksum256(verify_info->work)
                                                + bitcoin::be_uint_from_checksum256(*parent_cumulative_work));

        verify_info = std::nullopt;
        // next action
        if (!block_bucket_itr->miner || synchronizer == block_bucket_itr->miner) {
            status = verify_pass;
        } else {
            status = waiting_miner_verification;
        }
    }

    if (status == waiting_miner_verification) {
        auto current_block = current_block_number();
        utxo_manage::config_table _config(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
        auto config = _config.get();
        // save the miner information that passed for the first time
        block_miner_table _block_miner(get_self(), height);
        auto block_miner_idx = _block_miner.get_index<"byhash"_n>();
        auto block_miner_itr = block_miner_idx.find(hash);
        if (block_miner_itr == block_miner_idx.end()) {
            _block_miner.emplace(get_self(), [&](auto& row) {
                row.id = _block_miner.available_primary_key();
                row.hash = hash;
                row.miner = block_bucket_itr->miner;
                row.block_num = current_block;
            });
        } else {
            check(block_miner_itr->block_num + config.num_miner_priority_blocks <= current_block,
                  "blksync.xsat::verify: waiting for miners to produce blocks first");
            status = verify_pass;
        }
    }

    if (status == verify_pass) {
        // save passed index
        passed_index_table _passed_index(get_self(), height);
        _passed_index.emplace(get_self(), [&](auto& row) {
            row.id = _passed_index.available_primary_key();
            row.hash = hash;
            row.bucket_id = block_bucket_itr->bucket_id;
            row.synchronizer = synchronizer;
        });

        // update latest height
        if (block_bucket_itr->miner) {
            pool::updateheight_action _updateheight(POOL_REGISTER_CONTRACT, {get_self(), "active"_n});
            _updateheight.send(block_bucket_itr->miner, height, block_bucket_itr->btc_miners);
        }

        utxo_manage::consensus_action _consensus(UTXO_MANAGE_CONTRACT, {get_self(), "active"_n});
        _consensus.send(height, hash);
    }

    // update block status
    block_bucket_idx.modify(block_bucket_itr, same_payer, [&](auto& row) {
        row.status = status;
        row.verify_info = verify_info;
        if (miner) {
            row.miner = miner;
        }
        if (btc_miners.size() > 0) {
            row.btc_miners = btc_miners;
        }
        if (cumulative_work != checksum256()) {
            row.cumulative_work = cumulative_work;
        }
    });
    return {.status = get_block_status_name(status), .block_hash = hash};
}

//@private
template <typename T, typename ITR>
block_sync::verify_block_result block_sync::check_fail(T& _block_chunk, const ITR block_chunk_itr, const string& state,
                                                       const checksum256& block_hash) {
    _block_chunk.modify(block_chunk_itr, same_payer, [&](auto& row) {
        row.status = verify_fail;
        row.reason = state;
        row.miner = name();
        row.btc_miners = {};
        row.verify_info = std::nullopt;
    });
    return block_sync::verify_block_result{
        .status = get_block_status_name(verify_fail),
        .reason = state,
        .block_hash = block_hash,
    };
}

void block_sync::find_miner(std::vector<bitcoin::core::transaction_output> outputs, name& miner,
                            vector<string>& btc_miners) {
    pool::miner_table _miner = pool::miner_table(POOL_REGISTER_CONTRACT, POOL_REGISTER_CONTRACT.value);
    auto miner_idx = _miner.get_index<"byminer"_n>();
    for (const auto output : outputs) {
        if (!miner) {
            miner = xsat::utils::get_op_return_eos_account(output.script.data);
        }

        std::vector<string> to;
        bitcoin::ExtractDestination(output.script.data, to);
        if (to.size() == 1) {
            btc_miners.push_back(to[0]);
            if (!miner) {
                auto miner_itr = miner_idx.find(xsat::utils::hash(to[0]));
                if (miner_itr != miner_idx.end()) {
                    miner = miner_itr->synchronizer;
                }
            }
        }
    }
}

uint64_t block_sync::next_bucket_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.bucket_id++;
    _global_id.set(global_id, get_self());
    return global_id.bucket_id;
}
