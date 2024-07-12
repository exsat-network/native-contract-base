#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <bitcoin/core/transaction.hpp>
#include "../internal/utils.hpp"

using namespace eosio;
using std::string;

class [[eosio::contract("blksync.xsat")]] block_sync : public contract {
   public:
    using contract::contract;

    typedef uint8_t block_status;
    static const block_status uploading = 1;
    static const block_status upload_complete = 2;
    static const block_status verify_merkle = 3;
    static const block_status verify_parent_hash = 4;
    static const block_status verify_fail = 5;
    static const block_status verify_pass = 6;

    /**
     * @brief global id table.
     * @scope `get_self()`
     *
     * @field bucket_id - `blockbuckets` Specifies the primary key of the table
     */
    struct [[eosio::table]] global_id_row {
        uint32_t bucket_id;
    };
    typedef eosio::singleton<"globalid"_n, global_id_row> global_id_table;

    /**
     * @brief block upload info struct.
     *
     * @field previous_block_hash - hash in internal byte order of the previous block’s header.
     * @field work - block workload
     * @field witness_reserve_value - witness reserve value in the block
     * @field witness_commitment -witness commitment in the block
     * @field has_witness - whether any of the transactions in the block contains witness
     * @field header_merkle - the merkle root of the block
     * @field relay_header_merkle - check header merkle relay data
     * @field relay_witness_merkle - check witness merkle relay data
     * @field num_transactions - the number of transactions in the block
     * @field processed_position - the location of the block that has been resolved
     * @field processed_transactions - the number of processed transactions
     *
     */
    struct verify_info_data {
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
    };

    /**
     * @brief block bucket table.
     * @scope `synchronizer`
     *
     * @field bucket_id - primary key, bucket_id is the scope associated with block.bucket
     * @field height - block height
     * @field size - block size
     * @field num_chunks - number of chunks
     * @field uploaded_size - the size of the uploaded data
     * @field uploaded_chunks - number of chunks that have been uploaded
     * @field endorsements - obtain the maximum number of endorsements
     * @field status - status of pending block (uploading, verify_pass, verify_fail, endorse_parse, endorse_fail)
     * @field reason - cause of failure
     * @field cumulative_work - the cumulative workload of the block
     * @field miner - block miner address
     * @field verify_info - @see verify_info_data
     *
     */
    struct [[eosio::table]] block_bucket_row {
        uint64_t bucket_id;
        uint64_t height;
        checksum256 hash;
        uint32_t size;
        uint32_t uploaded_size;
        uint8_t num_chunks;
        uint8_t uploaded_num_chunks;
        block_status status;
        std::string reason;

        checksum256 cumulative_work;
        name miner;
        vector<string> btc_miners;
        std::optional<verify_info_data> verify_info;

        bool in_verifiable() const {
            return status == upload_complete || status == verify_merkle || status == verify_parent_hash;
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
     * @brief passed index table.
     * @scope `height`
     *
     * @field id - primary key
     * @field hash - block hash
     * @field bucket_id - primary key of blockbuckets table
     * @field synchronizer - account for uploading block data
     *
     */
    struct [[eosio::table]] passed_index_row {
        uint64_t id;
        checksum256 hash;
        uint64_t bucket_id;
        name synchronizer;
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
     * @brief block chunk table.
     * @scope `bucket_id`
     *
     * @field id - primary key
     * @field data - the block chunk for block
     *
     */
    struct [[eosio::table]] block_chunk_row {
        uint64_t id;
        std::vector<char> data;
        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index<"block.chunk"_n, block_chunk_row> block_chunk_table;

    struct verify_block_result {
        std::string status;
        checksum256 block_hash;
    };

    /**
     * consensus action.
     * @auth `utxomng.xsat`
     *
     * @param height - height of the consensus block
     * @param synchronizer - the synchronizer address
     * @param bucket_id - the block bucket id
     *
     */
    void consensus(const uint64_t height, const name &synchronizer, const uint64_t bucket_id);

    /**
     * delete bucket data action.
     * @auth `utxomng.xsat`
     *
     * @param height - height of the upload block
     * @param bucket_id - the block bucket id
     *
     */
    void delchunks(const uint64_t height, const uint64_t bucket_id);

    /**
     * init bucket action.
     * @auth `synchronizer`
     *
     * @param synchronizer - synchronizer account
     * @param height - Height of the upload block
     * @param hash - the hash of the uploaded block
     * @param block_size - chunk id (Start at 0)
     * @param num_chunks - the data of chunk
     *
     */
    [[eosio::action]]
    void initbucket(const name &synchronizer, const uint64_t height, const checksum256 &hash, const uint32_t block_size,
                    const uint8_t num_chunks);

    /**
     * push block chunk action.
     * @auth `synchronizer`
     *
     * @param synchronizer - synchronizer account
     * @param height -Height of the upload block
     * @param hash - the hash of the uploaded block
     * @param chunk_id - chunk id (Start at 0)
     * @param data - the data of chunk
     *
     */
    [[eosio::action]]
    void pushchunk(const name &synchronizer, const uint64_t height, const checksum256 &hash, const uint8_t chunk_id,
                   const std::vector<char> &data);

    /**
     * delete block chunk action.
     * @auth `from`
     *
     * @param synchronizer - synchronizer account
     * @param height - the height of the block to be deleted
     * @param hash - the hash of the block to delete
     * @param chunk_id - the chunk id of the block to be deleted
     *
     */
    [[eosio::action]]
    void delchunk(const name &synchronizer, const uint64_t height, const checksum256 &hash, const uint8_t chunk_id);

    /**
     * delete block data action.
     * @auth `synchronizer`
     *
     * @param synchronizer - synchronizer account
     * @param height - the height of the block to be deleted
     * @param hash - the hash of the block to delete
     *
     */
    [[eosio::action]]
    void delbucket(const name &synchronizer, const uint64_t height, const checksum256 &hash);

    /**
     * verify block action.
     * @auth `from`
     * @note from => the `uploader` whitelist in config.exsat
     *
     * @param from - Caller account
     * @param height - the height of the block to be verified
     * @param hash - the hash of the block to verified
     *
     */
    [[eosio::action]]
    verify_block_result verify(const name &from, const uint64_t height, const checksum256 &hash);

    // logs
    [[eosio::action]]
    void bucketlog(const uint64_t bucket_id, const name &synchronizer, const uint64_t height, const checksum256 &hash,
                   const uint32_t block_size, const uint8_t num_chunks) {
        require_auth(get_self());
    }

    using consensus_action = eosio::action_wrapper<"consensus"_n, &block_sync::consensus>;
    using delchunks_action = eosio::action_wrapper<"delchunks"_n, &block_sync::delchunks>;
    using bucketlog_action = eosio::action_wrapper<"bucketlog"_n, &block_sync::bucketlog>;

    // [start, end)
    static std::vector<char> read_bucket(const uint64_t bucket_id, const name &code, const uint64_t start,
                                         const uint64_t end) {
        block_chunk_table _block_chunk = block_chunk_table(code, bucket_id);
        auto begin_itr = _block_chunk.begin();
        auto end_itr = _block_chunk.end();

        std::vector<char> result;
        uint64_t last_position = 0;
        while (begin_itr != end_itr) {
            auto next_position = last_position + static_cast<std::uint64_t>(begin_itr->data.size());
            if (start <= last_position && end >= next_position) {
                result.insert(result.end(), begin_itr->data.begin(), begin_itr->data.end());
            } else if (start <= last_position && end < next_position) {
                result.insert(result.end(), begin_itr->data.begin(), begin_itr->data.end() - (next_position - end));
                break;
            } else if (start > last_position && end >= next_position) {
                result.insert(result.end(), begin_itr->data.begin() + (start - last_position), begin_itr->data.end());
            }
            last_position = next_position;
            begin_itr++;
        }
        return result;
    }

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
            case verify_pass:
                return "verify_pass";
            case verify_fail:
                return "verify_fail";
            default:
                return "invalid_status";
        }
    }

   private:
    // table init
    global_id_table _global_id = global_id_table(_self, _self.value);

    uint64_t next_bucket_id();

    void find_miner(std::vector<bitcoin::core::transaction_output> outputs, name &miner, vector<string> &btc_miners);
    template <typename T, typename ITR>
    verify_block_result check_fail(T &_block_chunk, const ITR block_chunk_itr, const string &state,
                                   const checksum256 &block_hash);
};
