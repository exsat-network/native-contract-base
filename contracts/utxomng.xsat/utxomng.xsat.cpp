#include <utxomng.xsat/utxomng.xsat.hpp>
#include <blksync.xsat/blksync.xsat.hpp>
#include <blkendt.xsat/blkendt.xsat.hpp>
#include <rwddist.xsat/rwddist.xsat.hpp>
#include <rescmng.xsat/rescmng.xsat.hpp>
#include <poolreg.xsat/poolreg.xsat.hpp>
#include <bitcoin/core/block_header.hpp>
#include <bitcoin/core/transaction.hpp>
#include <bitcoin/script/address.hpp>

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

//@auth get_self()
[[eosio::action]]
void utxo_manage::init(const uint64_t height, const checksum256& hash, const checksum256& cumulative_work) {
    require_auth(get_self());

    check(!_chain_state.exists() || _chain_state.get().irreversible_height == 0,
          "utxo_manage::init: can only be initialized once");
    check(height == START_HEIGHT, "utxo_manage::init: block height must be 839999");
    check(hash > checksum256(), "utxo_manage::init: invalid hash");
    check(cumulative_work > checksum256(), "utxo_manage::init: invalid cumulative_work");

    auto chain_state = _chain_state.get_or_default();
    chain_state.head_height = height;
    chain_state.irreversible_height = height;
    chain_state.irreversible_hash = hash;
    chain_state.status = waiting;
    _chain_state.set(chain_state, get_self());
}

//@auth get_self()
[[eosio::action]]
void utxo_manage::config(const uint16_t parse_timeout_seconds, const uint16_t num_validators_per_distribution,
                         const uint16_t num_retain_data_blocks, const uint8_t num_merkle_layer,
                         const uint16_t num_miner_priority_blocks) {
    require_auth(get_self());

    check(parse_timeout_seconds > 0, "utxo_manage::config: parse_timeout_seconds must be greater than 0");
    check(num_validators_per_distribution > 0,
          "utxo_manage::config: num_validators_per_distribution must be greater than 0");
    check(num_merkle_layer > 0, "utxo_manage::config: num_merkle_layer must be greater than 0");

    auto config = _config.get_or_default();
    config.parse_timeout_seconds = parse_timeout_seconds;
    config.num_validators_per_distribution = num_validators_per_distribution;
    config.num_retain_data_blocks = num_retain_data_blocks;
    config.num_merkle_layer = num_merkle_layer;
    config.num_txs_per_verification = 2 << (num_merkle_layer - 1);
    config.num_miner_priority_blocks = num_miner_priority_blocks;
    _config.set(config, get_self());
}

//@auth get_self()
[[eosio::action]]
void utxo_manage::addutxo(const uint64_t id, const checksum256& txid, const uint32_t index,
                          const vector<uint8_t>& scriptpubkey, const uint64_t value) {
    require_auth(get_self());

    auto utxo_idx = _utxo.get_index<"byutxoid"_n>();
    auto utxo_itr = utxo_idx.find(compute_utxo_id(txid, index));
    if (utxo_itr == utxo_idx.end()) {
        _utxo.emplace(get_self(), [&](auto& row) {
            row.id = id;
            row.txid = txid;
            row.index = index;
            row.scriptpubkey = scriptpubkey;
            row.value = value;
        });
        auto chain_state = _chain_state.get_or_default();
        chain_state.num_utxos += 1;
        _chain_state.set(chain_state, get_self());
    } else {
        utxo_idx.modify(utxo_itr, same_payer, [&](auto& row) {
            row.scriptpubkey = scriptpubkey;
            row.value = value;
        });
    }
}

//@auth get_self()
[[eosio::action]]
void utxo_manage::delutxo(const uint64_t id) {
    require_auth(get_self());

    auto& utxo = _utxo.get(id, "utxo_manage::delutxo: [utxos] does not exist");
    _utxo.erase(utxo);

    auto chain_state = _chain_state.get_or_default();
    chain_state.num_utxos -= 1;
    _chain_state.set(chain_state, get_self());
}

//@auth get_self()
[[eosio::action]]
void utxo_manage::addblock(const uint64_t height, const checksum256& hash, const checksum256& cumulative_work,
                           const uint32_t version, const checksum256& previous_block_hash, const checksum256& merkle,
                           const uint32_t timestamp, const uint32_t bits, const uint32_t nonce) {
    require_auth(get_self());
    check(height <= START_HEIGHT, "utxo_manage::addblock: height must be less than or equal to 839999");

    auto block_itr = _block.find(height);
    if (block_itr == _block.end()) {
        _block.emplace(get_self(), [&](auto& row) {
            row.height = height;
            row.hash = hash;
            row.cumulative_work = cumulative_work;
            row.version = version;
            row.previous_block_hash = previous_block_hash;
            row.merkle = merkle;
            row.timestamp = timestamp;
            row.bits = bits;
            row.nonce = nonce;
        });
    } else {
        _block.modify(block_itr, same_payer, [&](auto& row) {
            row.height = height;
            row.hash = hash;
            row.cumulative_work = cumulative_work;
            row.version = version;
            row.previous_block_hash = previous_block_hash;
            row.merkle = merkle;
            row.timestamp = timestamp;
            row.bits = bits;
            row.nonce = nonce;
        });
    }
}

//@auth get_self()
[[eosio::action]]
void utxo_manage::delblock(const uint64_t height) {
    require_auth(get_self());
    check(height <= START_HEIGHT, "utxo_manage::delblock: height must be less than or equal to 839999");

    auto block_itr = _block.require_find(height, "utxo_manage::delblock: [blocks] does not exist");
    _block.erase(block_itr);
}

//@auth blksync.xsat or blkendt.xsat
[[eosio::action]]
void utxo_manage::consensus(const uint64_t height, const checksum256& hash) {
    if (!has_auth(BLOCK_SYNC_CONTRACT)) {
        require_auth(BLOCK_ENDORSE_CONTRACT);
    }

    block_sync::passed_index_table _passed_index(BLOCK_SYNC_CONTRACT, height);
    auto passed_index_idx = _passed_index.get_index<"byhash"_n>();
    auto passed_index_itr = passed_index_idx.find(hash);

    if (passed_index_itr == passed_index_idx.end()) {
        return;
    }

    block_endorse::endorsement_table _endorsement(BLOCK_ENDORSE_CONTRACT, height);
    auto endorsement_idx = _endorsement.get_index<"byhash"_n>();
    auto endorsement_itr = endorsement_idx.find(hash);

    if (endorsement_itr == endorsement_idx.end() || !endorsement_itr->reached_consensus()) {
        return;
    }

    // Verify whether it exceeds the number of blocks miners preferentially produce.
    if (passed_index_itr->miner && passed_index_itr->synchronizer != passed_index_itr->miner) {
        block_sync::block_miner_table _block_miner(BLOCK_SYNC_CONTRACT, height);
        auto block_miner_idx = _block_miner.get_index<"byhash"_n>();
        auto block_miner_itr = block_miner_idx.require_find(hash);

        if (block_miner_itr->expired_block_num > current_block_number()) {
            return;
        }
    }

    // get header
    auto block_data = block_sync::read_bucket(passed_index_itr->bucket_id, BLOCK_SYNC_CONTRACT, 0, BLOCK_HEADER_SIZE);
    eosio::datastream<const char*> block_stream(block_data.data(), block_data.size());
    bitcoin::core::block_header block_header;
    block_stream >> block_header;

    _consensus_block.emplace(get_self(), [&](auto& row) {
        row.height = height;
        row.hash = hash;
        row.version = block_header.version;
        row.previous_block_hash = bitcoin::be_checksum256_from_uint(block_header.previous_block_hash);
        row.merkle = bitcoin::be_checksum256_from_uint(block_header.merkle);
        row.timestamp = block_header.timestamp;
        row.bits = block_header.bits;
        row.nonce = block_header.nonce;
        row.cumulative_work = passed_index_itr->cumulative_work;
        row.miner = passed_index_itr->miner;
        row.bucket_id = passed_index_itr->bucket_id;
        row.synchronizer = passed_index_itr->synchronizer;
    });

    // update chain state
    auto chain_state = _chain_state.get_or_default();
    if (chain_state.head_height < height) {
        chain_state.head_height = height;

        if (chain_state.head_height - chain_state.irreversible_height >= IRREVERSIBLE_BLOCKS
            && chain_state.parsing_height == 0) {
            find_set_next_block(&chain_state);
            auto config = _config.get();
            chain_state.parsed_expiration_time = current_time_point() + eosio::seconds(config.parse_timeout_seconds);
        }
        _chain_state.set(chain_state, get_self());
    }

    // consensus
    block_sync::consensus_action block_sync_consensus(BLOCK_SYNC_CONTRACT, {get_self(), "active"_n});
    block_sync_consensus.send(height, passed_index_itr->synchronizer, passed_index_itr->bucket_id);
}

//@auth
[[eosio::action]]
utxo_manage::process_block_result utxo_manage::processblock(const name& synchronizer, uint64_t process_row) {
    require_auth(synchronizer);

    auto current_time = current_time_point();

    auto chain_state = _chain_state.get_or_default();
    // get most work block
    if (chain_state.status == waiting) {
        check(chain_state.head_height - chain_state.irreversible_height >= IRREVERSIBLE_BLOCKS,
              "utxo_manage::processblock: must be more than " + std::to_string(IRREVERSIBLE_BLOCKS - 1)
                  + " blocks to process");

        find_set_next_block(&chain_state);

        // issue reward
        reward_distribution::distribute_action _distribute(REWARD_DISTRIBUTION_CONTRACT, {get_self(), "active"_n});
        _distribute.send(chain_state.parsing_height);

        // next action
        chain_state.status = parsing;
    }
    auto height = chain_state.parsing_height;
    auto hash = chain_state.parsing_hash;

    auto config = _config.get();

    // fee deduction
    resource_management::pay_action pay(RESOURCE_MANAGE_CONTRACT, {get_self(), "active"_n});
    pay.send(height, hash, synchronizer, PARSE, 1);

    // verify permissions and whether parsing times out
    if (chain_state.parsed_expiration_time > current_time) {
        check(synchronizer == chain_state.parser,
              "utxo_manage::processblock: you are not a parser of the current block");
    } else {
        pool::synchronizer_table _synchronizer(POOL_REGISTER_CONTRACT, POOL_REGISTER_CONTRACT.value);
        _synchronizer.require_find(synchronizer.value, "utxo_manage::processblock: only synchronizers can parse");

        chain_state.parser = synchronizer;
        chain_state.parsed_expiration_time = current_time + eosio::seconds(config.parse_timeout_seconds);
    }

    // parsing utxo
    if (chain_state.status == parsing) {
        process_transactions(&chain_state, process_row);

        // next action
        if (chain_state.num_transactions == chain_state.processed_transactions) {
            chain_state.status = deleting_data;
        }
    } else if (chain_state.status == deleting_data) {
        block_sync::delchunks_action _delchunks(BLOCK_SYNC_CONTRACT, {get_self(), "active"_n});

        // erase endorsement
        block_endorse::erase_action _erase(BLOCK_ENDORSE_CONTRACT, {get_self(), "active"_n});
        _erase.send(height);

        // erase old block chunks
        auto del_height = height - config.num_retain_data_blocks;
        block_extra_table _erase_block_extra(get_self(), del_height);
        if (_erase_block_extra.exists()) {
            _delchunks.send(_erase_block_extra.get().bucket_id);
            _erase_block_extra.remove();
        }

        // erase consensus block
        consensus_block_row consensus_block;
        auto consensus_block_idx = _consensus_block.get_index<"byheight"_n>();
        auto consensus_block_itr = consensus_block_idx.lower_bound(height);
        auto consensus_block_end = consensus_block_idx.upper_bound(height);
        while (consensus_block_itr != consensus_block_end) {
            // Without deleting the data of the current latest irreversible block, config.num_retain_data_blocks block
            // data needs to be retained.
            if (consensus_block_itr->bucket_id != chain_state.parsing_bucket_id) {
                _delchunks.send(consensus_block_itr->bucket_id);
            } else {
                consensus_block = *consensus_block_itr;
            }
            consensus_block_itr = consensus_block_idx.erase(consensus_block_itr);
        }

        // save irreversible block
        _block.emplace(get_self(), [&](auto& row) {
            row.height = consensus_block.height;
            row.hash = consensus_block.hash;
            row.cumulative_work = consensus_block.cumulative_work;
            row.version = consensus_block.version;
            row.previous_block_hash = consensus_block.previous_block_hash;
            row.merkle = consensus_block.merkle;
            row.timestamp = consensus_block.timestamp;
            row.bits = consensus_block.bits;
            row.nonce = consensus_block.nonce;
        });

        // save block extra
        block_extra_table _block_extra = block_extra_table(get_self(), height);
        auto block_extra = _block_extra.get_or_default();
        block_extra.bucket_id = chain_state.parsing_bucket_id;
        _block_extra.set(block_extra, get_self());

        // next action
        chain_state.status = distributing_rewards;

    } else if (chain_state.status == distributing_rewards) {
        reward_distribution::reward_log_table _reward_log(REWARD_DISTRIBUTION_CONTRACT,
                                                          REWARD_DISTRIBUTION_CONTRACT.value);
        auto reward_log_itr = _reward_log.require_find(height);

        auto from_index = chain_state.num_validators_assigned;
        auto to_index = from_index + config.num_validators_per_distribution;
        if (to_index > chain_state.num_provider_validators) {
            to_index = chain_state.num_provider_validators;
        }
        chain_state.num_validators_assigned = to_index;

        // distribute rewards to validators in batches
        reward_distribution::endtreward_action _endteward(REWARD_DISTRIBUTION_CONTRACT, {get_self(), "active"_n});
        _endteward.send(chain_state.parser, height, from_index, to_index);

        // next height
        if (chain_state.num_provider_validators == chain_state.num_validators_assigned) {
            chain_state.irreversible_height = height;
            chain_state.irreversible_hash = hash;
            chain_state.status = waiting;
            chain_state.num_transactions = 0;
            chain_state.processed_transactions = 0;
            chain_state.processed_position = 0;
            chain_state.processed_vin = 0;
            chain_state.processed_vout = 0;
            chain_state.parser = {};
            chain_state.parsing_bucket_id = 0;
            chain_state.parsing_height = 0;
            chain_state.parsing_hash = checksum256();
            chain_state.num_provider_validators = 0;
            chain_state.num_validators_assigned = 0;
            chain_state.synchronizer = {};
            chain_state.miner = {};
            chain_state.parsed_expiration_time = time_point_sec::min();
            if (chain_state.head_height - chain_state.irreversible_height >= IRREVERSIBLE_BLOCKS) {
                find_set_next_block(&chain_state);
                chain_state.parsed_expiration_time = current_time + eosio::seconds(config.parse_timeout_seconds);
            }
        }
    }

    // save state
    _chain_state.set(chain_state, get_self());

    return {.height = height, .block_hash = hash, .status = get_parsing_status_name(chain_state.status)};
}

void utxo_manage::find_set_next_block(utxo_manage::chain_state_row* chain_state) {
    auto consensus_block
        = find_next_irreversible_block(chain_state->irreversible_height, chain_state->irreversible_hash);
    chain_state->parsing_bucket_id = consensus_block.bucket_id;
    chain_state->parsing_height = consensus_block.height;
    chain_state->parsing_hash = consensus_block.hash;
    chain_state->miner = consensus_block.miner;
    chain_state->synchronizer = consensus_block.synchronizer;
    chain_state->parser = consensus_block.synchronizer;

    block_endorse::endorsement_table _endorsement(BLOCK_ENDORSE_CONTRACT, chain_state->parsing_height);
    auto endorsement_idx = _endorsement.get_index<"byhash"_n>();
    auto endorsement_itr = endorsement_idx.require_find(chain_state->parsing_hash);
    chain_state->num_provider_validators = endorsement_itr->provider_validators.size();
}

utxo_manage::consensus_block_row utxo_manage::find_next_irreversible_block(const uint64_t irreversible_height,
                                                                           const checksum256& irreversible_hash) {
    const auto err_msg = "utxo_manage::processblock: next irreversible block not found";
    // If the next block does not fork, return directly
    auto height_idx = _consensus_block.get_index<"byheight"_n>();
    auto consensus_block_itr = height_idx.require_find(irreversible_height + 1, err_msg);
    if (std::distance(consensus_block_itr, height_idx.end()) == 1) {
        return *consensus_block_itr;
    }

    // Find the parent block with the largest cumulative work after 6 blocks
    consensus_block_row parent;
    auto parent_height = irreversible_height + IRREVERSIBLE_BLOCKS;
    auto parent_itr = height_idx.lower_bound(parent_height);
    auto parent_end = height_idx.upper_bound(parent_height);
    check(parent_itr != parent_end, err_msg);
    while (parent_itr != parent_end) {
        if (parent.cumulative_work < parent_itr->cumulative_work) {
            parent = *parent_itr;
        }
        parent_itr++;
    }

    // Backtrack from the parent block to the irreversible block
    auto block_id_idx = _consensus_block.get_index<"byblockid"_n>();
    auto irreversible_block = block_id_idx.require_find(
        xsat::utils::compute_block_id(parent.height - 1, parent.previous_block_hash), err_msg);
    while (irreversible_block != block_id_idx.end() && irreversible_block->previous_block_hash != irreversible_hash) {
        irreversible_block = block_id_idx.require_find(
            xsat::utils::compute_block_id(irreversible_block->height - 1, irreversible_block->previous_block_hash),
            err_msg);
    }
    check(irreversible_block->previous_block_hash == irreversible_hash, err_msg);
    return *irreversible_block;
}

void utxo_manage::process_transactions(utxo_manage::chain_state_row* chain_state, uint64_t process_row) {
    auto block_data = block_sync::read_bucket(chain_state->parsing_bucket_id, BLOCK_SYNC_CONTRACT,
                                              BLOCK_HEADER_SIZE + chain_state->processed_position,
                                              std::numeric_limits<uint64_t>::max());
    eosio::datastream<const char*> block_stream(block_data.data(), block_data.size());

    if (chain_state->processed_position == 0) {
        chain_state->num_transactions = bitcoin::varint::decode(block_stream);
    }
    if (process_row == 0) process_row = std::numeric_limits<uint64_t>::max();

    uint64_t processed_position = 0;
    auto pending_transactions = chain_state->num_transactions - chain_state->processed_transactions;
    while (pending_transactions-- && process_row) {
        bitcoin::core::transaction transaction;
        block_stream >> transaction;
        auto txid = bitcoin::be_checksum256_from_uint(transaction.merkle_hash());

        // save vout
        for (; chain_state->processed_vout < transaction.outputs.size() && process_row;
             chain_state->processed_vout++, process_row--) {
            auto vout = transaction.outputs[chain_state->processed_vout];
            if (vout.value == 0) continue;
            save_utxo(vout.script.data, vout.value, txid, chain_state->processed_vout);

            chain_state->num_utxos += 1;
        }

        // remove vin
        for (; chain_state->processed_vin < transaction.inputs.size() && process_row;
             chain_state->processed_vin++, process_row--) {
            auto vin = transaction.inputs[chain_state->processed_vin];
            if (transaction.is_coinbase()) continue;

            auto hash_prev_utxo
                = remove_utxo(bitcoin::be_checksum256_from_uint(vin.previous_output_hash), vin.previous_output_index);
            if (hash_prev_utxo) {
                chain_state->num_utxos -= 1;
            }
        }

        // next transaction
        if (chain_state->processed_vin == transaction.inputs.size()
            && chain_state->processed_vout == transaction.outputs.size()) {
            processed_position = block_stream.tellp();
            chain_state->processed_vin = 0;
            chain_state->processed_vout = 0;
            chain_state->processed_transactions++;
        }
    }
    chain_state->processed_position += processed_position;
}

bool utxo_manage::remove_utxo(const checksum256& prev_txid, const uint32_t prev_index) {
    auto utxo_idx = _utxo.get_index<"byutxoid"_n>();

    auto utxo_itr = utxo_idx.find(compute_utxo_id(prev_txid, prev_index));
    if (utxo_itr != utxo_idx.end()) {
        utxo_idx.erase(utxo_itr);
        return true;
    } else {
        // log
        utxo_manage::lostutxolog_action _lostutxolog(get_self(), {get_self(), "active"_n});
        _lostutxolog.send(prev_txid, prev_index);
        return false;
    }
}

utxo_manage::utxo_row utxo_manage::save_utxo(const std::vector<uint8_t>& script_data, const uint64_t value,
                                             const checksum256& txid, const uint32_t index) {
    //  save output
    auto id = _utxo.available_primary_key();
    if (id == 0) {
        id = 1;
    }
    auto utxo_itr = _utxo.emplace(get_self(), [&](auto& row) {
        row.id = id;
        row.txid = txid;
        row.index = index;
        row.scriptpubkey = script_data;
        row.value = value;
    });
    return *utxo_itr;
}