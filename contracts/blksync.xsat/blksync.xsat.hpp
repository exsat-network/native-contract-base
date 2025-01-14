#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <eosio/binary_extension.hpp>
#include <bitcoin/core/transaction.hpp>
#include "../internal/utils.hpp"

using namespace eosio;
using namespace std;

class [[eosio::contract("blksync.xsat")]] block_sync : public contract {
   public:
    using contract::contract;

    typedef uint8_t block_status;
    static const block_status uploading = 1;
    static const block_status upload_complete = 2;
    static const block_status verify_merkle = 3;
    static const block_status verify_parent_hash = 4;
    static const block_status waiting_miner_verification = 5;
    static const block_status verify_fail = 6;
    static const block_status verify_pass = 7;

    static std::string get_block_status_name(const block_status status) {
        switch (status) {
            case uploading:
                return "uploading";
            case upload_complete:
                return "upload_complete";
            case verify_merkle:
                return "verify_merkle";
            case verify_parent_hash:
                return "verify_parent_hash";
            case waiting_miner_verification:
                return "waiting_miner_verification";
            case verify_pass:
                return "verify_pass";
            case verify_fail:
                return "verify_fail";
            default:
                return "invalid_status";
        }
    }

    struct out_point {
        bitcoin::uint256_t tx_id;
        uint32_t index;

        friend bool operator<(const out_point &a, const out_point &b) {
            return std::tie(a.tx_id, a.index) < std::tie(b.tx_id, b.index);
        }

        friend bool operator==(const out_point &a, const out_point &b) {
            return (a.tx_id == b.tx_id && a.index == b.index);
        }
    };

    /**
     * ## TABLE `globalid`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} bucket_id` - latest bucket_id
     *
     * ### example
     *
     * ```json
     * {
     *   "bucket_id": 1
     * }
     * ```
     */
    struct [[eosio::table]] global_id_row {
        uint64_t bucket_id;
    };
    typedef eosio::singleton<"globalid"_n, global_id_row> global_id_table;

    /**
     * ## STRUCT `verify_info_data`
     *
     * ### params
     *
     * - `{name} miner` - block miner account
     * - `{vector<string>} btc_miners` - btc miner account
     * - `{checksum256} previous_block_hash` - hash in internal byte order of the previous block’s header
     * - `{checksum256} work` - block workload
     * - `{checksum256} witness_reserve_value` - witness reserve value in the block
     * - `{std::optional<checksum256>}` - witness commitment in the block
     * - `{bool} has_witness` - whether any of the transactions in the block contains witness
     * - `{checksum256} header_merkle` - the merkle root of the block
     * - `{std::vector<checksum256>} relay_header_merkle` - check header merkle relay data
     * - `{std::vector<checksum256>} relay_witness_merkle` - check witness merkle relay data
     * - `{uint64_t} num_transactions` - the number of transactions in the block
     * - `{uint64_t} processed_position` - the location of the block that has been resolved
     * - `{uint64_t} processed_transactions` - the number of processed transactions
     * - `{uint32_t} timestamp` - the block time in seconds since epoch (Jan 1 1970 GMT)
     * - `{uint32_t} bits` - the bits
     *
     * ### example
     *
     * ```json
     * {
     *   "miner": "",
     *   "btc_miners": [
     *       "1BM1sAcrfV6d4zPKytzziu4McLQDsFC2Qc"
     *   ],
     *   "previous_block_hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e",
     *   "work": "000000000000000000000000000000000000000000004e9235f043634662e0cb",
     *   "witness_reserve_value": "0000000000000000000000000000000000000000000000000000000000000000",
     *   "witness_commitment": "aeaa22969e5aac88afd1ac14b19a3ad3a58f5eb0dd151ddddfc749297ebfb020",
     *   "has_witness": 1,
     *   "header_merkle": "f3f07d3e4636fa1ae5300b3bc148c361beafd7b3309d30b7ba136d0e59a9a0e5",
     *   "relay_header_merkle": [
     *      "d1c9861b0d129b34bb6b733c624bbe0a9b10ff01c6047dced64586ef584987f4",
     *      "bd95f641a29379f0b5a26961de4bb36bd9568a67ca0615be3fb0a28152ff1806",
     *      "667eb5d36c67667ae4f10bd30a62e3797e8700e1fbb5e3f754a7526f2b7db1e2",
     *      "5193ac78b5ef8f570ed24946fbcb96d71284faa27b86296093a93eb5c1cfac06"
     *   ],
     *   "relay_witness_merkle": [
     *      "8a080509ebf6baca260d466c2669200d9b4de750f6a190382c4e8ab6ab6859db",
     *      "d65d4261be51ca1193e718a6f0cfe6415b6f122f4c3df87861e7452916b45d78",
     *      "95aa96164225b76afa32a9b2903241067b0ea71228cc2d51b9321148c4e37dd3",
     *      "0dfca7530a6e950ecdec67c60e5d9574404cc97b333a4e24e3cf2eadd5eb76bd"
     *   ],
     *   "num_transactions": 4899,
     *   "processed_transactions": 4096,
     *   "processed_position": 1197889,
     *   "timestamp": 1713608213,
     *   "bits": 386089497
     * }
     * ```
     */
    struct verify_info_data {
        name miner;
        vector<string> btc_miners;
        checksum256 previous_block_hash;
        checksum256 work;
        std::optional<checksum256> witness_reserve_value;
        std::optional<checksum256> witness_commitment;
        bool has_witness;
        checksum256 header_merkle;
        std::vector<checksum256> relay_header_merkle;
        std::vector<checksum256> relay_witness_merkle;
        uint64_t num_transactions = 0;
        uint64_t processed_transactions = 0;
        uint64_t processed_position = 0;
        uint32_t timestamp;
        uint32_t bits;
    };

    /**
     * ## TABLE `blockbuckets`
     *
     * ### scope `validator`
     * ### params
     *
     * - `{uint64_t} bucket_id` - primary key, bucket_id is the scope associated with block.bucket
     * - `{uint64_t} height` - block height
     * - `{uint32_t} size` -block size
     * - `{uint32_t} uploaded_size` - the latest release id
     * - `{uint8_t} num_chunks` - number of chunks
     * - `{uint8_t} uploaded_num_chunks` - number of chunks that have been uploaded
     * - `{uint32_t} chunk_size` - the size of each chunk
     * - `{vector<uint8_t>} chunk_ids` - the uploaded chunk_id
     * - `{string} reason` - reason for verification failure
     * - `{block_status} status` - current block status
     * - `{time_point_sec} updated_at` - updated at time
     * - `{std::optional<verify_info_data>} verify_info` - @see struct `verify_info_data`
     *
     * ### example
     *
     * ```json
     * {
     *   "bucket_id": 81,
     *   "height": 840062,
     *   "hash": "00000000000000000002fc5099a59501b26c34819ac52cc16141275f158c3c6a",
     *   "size": 1434031,
     *   "uploaded_size": 1434031,
     *   "num_chunks": 11,
     *   "uploaded_num_chunks": 11,
     *   "chunk_size": 256000,
     *   "chunk_ids": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
     *   "reason": "",
     *   "status": 3,
     *   "updated_at": "2024-08-19T00:00:00",
     *   "verify_info": {
     *       "miner": "",
     *       "btc_miners": [
     *           "1BM1sAcrfV6d4zPKytzziu4McLQDsFC2Qc"
     *       ],
     *       "previous_block_hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e",
     *       "work": "000000000000000000000000000000000000000000004e9235f043634662e0cb",
     *       "witness_reserve_value": "0000000000000000000000000000000000000000000000000000000000000000",
     *       "witness_commitment": "aeaa22969e5aac88afd1ac14b19a3ad3a58f5eb0dd151ddddfc749297ebfb020",
     *       "has_witness": 1,
     *       "header_merkle": "f3f07d3e4636fa1ae5300b3bc148c361beafd7b3309d30b7ba136d0e59a9a0e5",
     *       "relay_header_merkle": [
     *          "d1c9861b0d129b34bb6b733c624bbe0a9b10ff01c6047dced64586ef584987f4",
     *          "bd95f641a29379f0b5a26961de4bb36bd9568a67ca0615be3fb0a28152ff1806",
     *          "667eb5d36c67667ae4f10bd30a62e3797e8700e1fbb5e3f754a7526f2b7db1e2",
     *          "5193ac78b5ef8f570ed24946fbcb96d71284faa27b86296093a93eb5c1cfac06"
     *       ],
     *       "relay_witness_merkle": [
     *           "8a080509ebf6baca260d466c2669200d9b4de750f6a190382c4e8ab6ab6859db",
     *           "d65d4261be51ca1193e718a6f0cfe6415b6f122f4c3df87861e7452916b45d78",
     *           "95aa96164225b76afa32a9b2903241067b0ea71228cc2d51b9321148c4e37dd3",
     *           "0dfca7530a6e950ecdec67c60e5d9574404cc97b333a4e24e3cf2eadd5eb76bd"
     *       ],
     *       "num_transactions": 4899,
     *       "processed_transactions": 4096,
     *       "processed_position": 1197889,
     *       "timestamp": 1713608213,
     *       "bits": 386089497
     *   }
     * }
     * ```
     */
    struct [[eosio::table]] block_bucket_row {
        uint64_t bucket_id;
        uint64_t height;
        checksum256 hash;
        uint32_t size;
        uint32_t uploaded_size;
        uint8_t num_chunks;
        uint8_t uploaded_num_chunks;
        uint32_t chunk_size;
        std::set<uint16_t> chunk_ids;
        std::string reason;
        block_status status;
        time_point_sec updated_at;

        std::optional<verify_info_data> verify_info;

        bool in_verifiable() const {
            return status == upload_complete || status == verify_merkle || status == verify_parent_hash
                   || status == waiting_miner_verification;
        }

        uint64_t primary_key() const { return bucket_id; }
        uint64_t by_status() const { return status; }
        checksum256 by_block_id() const { return xsat::utils::compute_block_id(height, hash); }
    };
    typedef eosio::multi_index<
        "blockbuckets"_n, block_bucket_row,
        eosio::indexed_by<"bystatus"_n, const_mem_fun<block_bucket_row, uint64_t, &block_bucket_row::by_status>>,
        eosio::indexed_by<"byblockid"_n, const_mem_fun<block_bucket_row, checksum256, &block_bucket_row::by_block_id>>>
        block_bucket_table;

    /**
     * ## TABLE `passedindexs`
     *
     * ### scope `height`
     * ### params
     *
     * - `{uint64_t} id` - primary key
     * - `{checksum256} hash` - block hash
     * - `{checksum256} cumulative_work` - the cumulative workload of the block
     * - `{uint64_t} bucket_id` - bucket_id is used to obtain block data
     * - `{name} synchronizer` - synchronizer account
     * - `{name} miner` - miner account
     * - `{time_point_sec} created_at` - created at time
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 0,
     *   "hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e",
     *   "cumulative_work": "0000000000000000000000000000000000000000753f3af9322a2a893cb6ece4",
     *   "bucket_id": 80,
     *   "synchronizer": "test.xsat",
     *   "miner": "alice",
     *   "created_at": "2024-08-13T00:00:00"
     * }
     * ```
     */
    struct [[eosio::table]] passed_index_row {
        uint64_t id;
        checksum256 hash;
        checksum256 cumulative_work;
        uint64_t bucket_id;
        name synchronizer;
        name miner;
        time_point_sec created_at;
        uint64_t primary_key() const { return id; }
        uint64_t by_bucket_id() const { return bucket_id; }
        checksum256 by_hash() const { return hash; }
    };

    typedef eosio::multi_index<
        "passedindexs"_n, passed_index_row,
        eosio::indexed_by<"bybucketid"_n, const_mem_fun<passed_index_row, uint64_t, &passed_index_row::by_bucket_id>>,
        eosio::indexed_by<"byhash"_n, const_mem_fun<passed_index_row, checksum256, &passed_index_row::by_hash>>>
        passed_index_table;

    /**
     * ## TABLE `blockminer`
     *
     * ### scope `height`
     * ### params
     *
     * - `{uint64_t} id` - primary key
     * - `{checksum256} hash` - block hash
     * - `{name} miner` - block miner account
     * - `{uint32_t} block_num` - the block number that passed the first verification
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 0,
     *   "hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e",
     *   "miner": "alice",
     *   "expired_block_num": 210000
     * }
     * ```
     */
    struct [[eosio::table]] block_miner_row {
        uint64_t id;
        checksum256 hash;
        name miner;
        uint32_t expired_block_num;
        uint64_t primary_key() const { return id; }
        checksum256 by_hash() const { return hash; }
    };
    typedef eosio::multi_index<
        "blockminer"_n, block_miner_row,
        eosio::indexed_by<"byhash"_n, const_mem_fun<block_miner_row, checksum256, &block_miner_row::by_hash>>>
        block_miner_table;

    /**
     * ## TABLE `block.chunk`
     *
     * ### scope `bucket_id`
     * ### params
     *
     * - `{std::vector<char>} data` - the block chunk for block
     *
     * ### example
     *
     * ```json
     * {
     *   "data": ""
     * }
     * ```
     */
    struct [[eosio::table]] block_chunk_row {
        std::vector<char> data;
    };
    typedef eosio::multi_index<"block.chunk"_n, block_chunk_row> block_chunk_table;

    /**
     * ## STRUCT `verify_block_result`
     *
     * ### params
     *
     * - `{string} status` - verification status (uploading, upload_complete, verify_merkle, verify_parent_hash, waiting_miner_verification, verify_pass, verify_fail)
     * - `{string} reason` - reason for verification failure
     * - `{checksum256} block_hash` - block hash
     *
     * ### example
     *
     * ```json
     * {
     *   "status": "verify_pass",
     *   "reason": "",
     *   "block_hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e"
     * }
     * ```
     */
    struct verify_block_result {
        std::string status;
        std::string reason;
        checksum256 block_hash;
    };

    /**
     * ## ACTION `consensus`
     *
     * - **authority**: `utxomng.xsat`
     *
     * > Consensus completion processing logic
     *
     * ### params
     *
     * - `{uint64_t} height` - block height
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} bucket_id` - bucket id
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blksync.xsat consensus '[840000, "alice", 1]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void consensus(const uint64_t height, const name &synchronizer, const uint64_t bucket_id);

    /**
     * ## ACTION `delchunks`
     *
     * - **authority**: `utxomng.xsat`
     *
     * > Deletion of historical block data after parsing is completed
     *
     * ### params
     *
     * - `{uint64_t} bucket_id` - bucket_id of block data to be deleted
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blksync.xsat delchunks '[1]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void delchunks(const uint64_t bucket_id);

    /**
     * ## ACTION `initbucket`
     *
     * - **authority**: `synchronizer`
     *
     * > Initialize the block information to be uploaded
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{uint32_t} size` -block size
     * - `{uint8_t} num_chunks` - number of chunks
     * - `{uint32_t} chunk_size` - the size of each chunk
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blksync.xsat initbucket '["alice", 840000, "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", 2325617, 9, 25600]' -p alice
     * ```
     */
    [[eosio::action]]
    void initbucket(const name &synchronizer, const uint64_t height, const checksum256 &hash, const uint32_t block_size,
                    const uint8_t num_chunks, const uint32_t chunk_size);

    /**
     * ## ACTION `pushchunk`
     *
     * - **authority**: `synchronizer`
     *
     * > Upload block shard data
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{uint8_t} chunk_id` - chunk id
     * - `{std::vector<char>} data` - block data to be uploaded
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blksync.xsat pushchunk '["alice", 840000, "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", 0, ""]' -p alice
     * ```
     */
    [[eosio::action]]
    void pushchunk(const name &synchronizer, const uint64_t height, const checksum256 &hash, const uint8_t chunk_id,
                   const eosio::ignore<std::vector<char>> &data);

    /**
     * ## ACTION `delchunk`
     *
     * - **authority**: `synchronizer`
     *
     * > Delete block shard data
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{uint8_t} chunk_id` - chunk id
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blksync.xsat delchunk '["alice", 840000, "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", 0]' -p alice
     * ```
     */
    [[eosio::action]]
    void delchunk(const name &synchronizer, const uint64_t height, const checksum256 &hash, const uint8_t chunk_id);

    /**
     * ## ACTION `delbucket`
     *
     * - **authority**: `synchronizer`
     *
     * > Delete the entire block data
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blksync.xsat delbucket '["alice", 840000, "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"]' -p alice
     * ```
     */
    [[eosio::action]]
    void delbucket(const name &synchronizer, const uint64_t height, const checksum256 &hash);

    /**
     * ## ACTION `verify`
     *
     * - **authority**: `synchronizer`
     *
     * > Verify block data

     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{uint64_t} nonce` - unique value for each call to prevent duplicate transactions 
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blksync.xsat verify '["alice", 840000, "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", 1]' -p alice
     * ```
     */
    [[eosio::action]]
    verify_block_result verify(const name &synchronizer, const uint64_t height, const checksum256 &hash,
                               const uint64_t nonce);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const name &synchronizer, const uint64_t height, const uint64_t bucket_id,
                    const optional<uint64_t> max_rows);

    [[eosio::action]]
    void reset(const name &synchronizer, const uint64_t height, const checksum256 &hash);

    [[eosio::action]]
    void updateparent(const name &synchronizer, const uint64_t height, const checksum256 &hash,
                      const checksum256 &parent);

    [[eosio::action]]
    void forkblock(const name &synchronizer, const uint64_t height, const checksum256 &hash, const name &account,
                   const uint32_t nonce);

    [[eosio::action]]
    uint32_t getbits(uint32_t timestamp, const uint64_t p_height, const uint32_t p_timestamp, const uint32_t p_bits);
#endif

    // logs
    [[eosio::action]]
    void bucketlog(const uint64_t bucket_id, const name &synchronizer, const uint64_t height, const checksum256 &hash,
                   const uint32_t block_size, const uint8_t num_chunks, const uint32_t chunk_size) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void chunklog(const uint64_t bucket_id, const uint8_t chunk_id, const uint8_t uploaded_num_chunks) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void delchunklog(const uint64_t bucket_id, const uint8_t chunk_id, const uint8_t uploaded_num_chunks) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void delbucketlog(const uint64_t bucket_id) {
        require_auth(get_self());
    }

    using consensus_action = eosio::action_wrapper<"consensus"_n, &block_sync::consensus>;
    using delchunks_action = eosio::action_wrapper<"delchunks"_n, &block_sync::delchunks>;
    using bucketlog_action = eosio::action_wrapper<"bucketlog"_n, &block_sync::bucketlog>;
    using chunklog_action = eosio::action_wrapper<"chunklog"_n, &block_sync::chunklog>;
    using delchunklog_action = eosio::action_wrapper<"delchunklog"_n, &block_sync::delchunklog>;
    using delbucketlog_action = eosio::action_wrapper<"delbucketlog"_n, &block_sync::delbucketlog>;

    static uint64_t compute_passed_index_id(const uint64_t block_id, const uint64_t miner_priority,
                                            const uint64_t pass_number) {
        // block_id (32 bit) + miner priority (8 bit) + pass number (24)
        return block_id << 32 | miner_priority << 24 | pass_number;
    }

    // [start, end)
    inline static std::vector<char> read_bucket(const eosio::name &code, const uint64_t bucket_id,
                                                const eosio::name &table, const uint64_t start, const uint64_t end) {
        // itr, size, from, to
        std::vector<std::tuple<int32_t, int32_t, int32_t, int32_t>> ranges;
        auto last_position = 0;
        auto total_size = 0;
        auto iter = eosio::internal_use_do_not_use::db_lowerbound_i64(code.value, bucket_id, table.value, 0);

        while (iter >= 0) {
            auto size = eosio::internal_use_do_not_use::db_get_i64(iter, nullptr, 0);
            auto next_position = last_position + size;
            if (start <= last_position && end >= next_position) {
                ranges.push_back(make_tuple(iter, size, 0, size));
                total_size += size;
            } else if (start <= last_position && end < next_position && end > last_position) {
                ranges.push_back(make_tuple(iter, size, 0, end - last_position));
                total_size += end - last_position;
                break;
            } else if (start > last_position && start < next_position && end >= next_position) {
                ranges.push_back(make_tuple(iter, size, start - last_position, size));
                total_size += next_position - start;
            } else if (start > last_position && start < next_position && end < next_position) {
                ranges.push_back(make_tuple(iter, size, start - last_position, end - last_position));
                total_size += end - start;
            }
            last_position += size;
            uint64_t ignored;
            iter = eosio::internal_use_do_not_use::db_next_i64(iter, &ignored);
        }
        std::vector<char> result;
        result.resize(total_size);
        size_t offset = 0;

        for (const auto &[iter, size, from, to] : ranges) {
            auto data_size = to - from;
            if (from == 0) {
                eosio::internal_use_do_not_use::db_get_i64(iter, result.data() + offset, data_size);
            } else {
                std::vector<char> data;
                data.resize(size);
                eosio::internal_use_do_not_use::db_get_i64(iter, data.data(), size);
                std::copy(data.begin() + from, data.begin() + to, result.begin() + offset);
            }
            offset += data_size;
        }
        return result;
    }

   private:
    // table init
    global_id_table _global_id = global_id_table(_self, _self.value);

    uint64_t next_bucket_id();

    void find_miner(std::vector<bitcoin::core::transaction_output> outputs, name &miner, vector<string> &btc_miners);

    optional<string> check_transaction(const bitcoin::core::transaction tx);

    template <typename ITR>
    optional<string> check_merkle(const ITR &block_bucket_itr, verify_info_data &verify_info);

    template <typename T, typename ITR>
    verify_block_result check_fail(T &_block_bucket, const ITR block_bucket_itr, const string &state,
                                   const checksum256 &block_hash);

#ifdef DEBUG
    template <typename T>
    void clear_table(T &table, uint64_t rows_to_clear);
#endif
};
