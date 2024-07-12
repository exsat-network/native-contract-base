#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include "../internal/defines.hpp"
#include "../internal/utils.hpp"

using namespace eosio;
using std::string;

class [[eosio::contract("utxomng.xsat")]] utxo_manage : public contract {
   public:
    using contract::contract;

    typedef uint8_t parsing_status;
    static const parsing_status waiting = 1;
    static const parsing_status parsing = 2;
    static const parsing_status deleting_data = 3;
    static const parsing_status distributing_rewards = 4;

    /**
     * @brief chain state table.
     * @scope `get_self()`
     *
     * @field head_height - header block height for consensus success
     * @field irreversible_height -irreversible block height
     * @field irreversible_hash - irreversible block hash
     * @field num_utxos - total number of UTXOs
     * @field status - parsing status @see parsing_status
     * @field parsing_hash - parsing block hash
     * @field num_transactions - the number of transactions in the block that was resolved, the value of which was not
     * parsed for the first time was 0
     * @field processed_position - the location of the block that has been resolved
     * @field processed_transactions - the number of resolved transactions
     * @field processed_vin - parse to the index of vin
     * @field processed_vout - parse to the index of vout
     * @field parser - current block resolution account
     * @field parsed_expiration_time - the timeout of the next block resolution, and the reward obtained by other
     * synchronizers can be resolved
     * @field miner - block miner address
     * @field synchronizer - block synchronizer address
     * @field num_provider_validators - the number of validators endorsed by the current block
     * @field num_validators_assigned - the number of validators that have been allocated rewards
     *
     */
    struct [[eosio::table("chainstate")]] chain_state_row {
        uint64_t head_height;
        uint64_t irreversible_height;
        checksum256 irreversible_hash;
        uint64_t num_utxos;
        parsing_status status;
        uint64_t parsing_height;
        checksum256 parsing_hash;
        uint64_t parsing_bucket_id;
        uint64_t num_transactions;
        uint64_t processed_transactions;
        uint64_t processed_position;
        uint64_t processed_vin;
        uint64_t processed_vout;
        uint32_t num_provider_validators;
        uint32_t num_validators_assigned;
        name miner;
        name synchronizer;
        name parser;
        eosio::time_point_sec parsed_expiration_time;
    };
    typedef eosio::singleton<"chainstate"_n, chain_state_row> chain_state_table;

    /**
     * @brief chain state table.
     * @scope `get_self()`
     *
     * @field parse_timeout_seconds - parsing timeout duration
     * @field num_validators_per_distribution - number of endorsing users each time rewards are distributed
     * @field num_retain_data_blocks - number of blocks to retain data
     * @field num_txs_per_verification - the number of tx for each verification (2^n)
     * @field num_merkle_layer - verify the number of merkle levels (log(num_txs_per_verification))
     *
     * */
    struct [[eosio::table("config")]] config_row {
        uint16_t parse_timeout_seconds = 600;
        uint16_t num_validators_per_distribution = 100;
        uint16_t num_retain_data_blocks = 100;
        uint16_t num_txs_per_verification = 2048;
        uint8_t num_merkle_layer = 11;
    };
    typedef eosio::singleton<"config"_n, config_row> config_table;

    /**
     * @brief utxo table.
     * @scope `get_self()`
     *
     * @field id - the height of the block in resolution
     * @field txid - transaction id
     * @field index - vout index
     * @field to - the account of utxo receiver
     * @field value - utxo quantity
     *
     */
    struct [[eosio::table]] utxo_row {
        uint64_t id;
        checksum256 txid;
        uint32_t index;
        std::vector<uint8_t> scriptpubkey;
        uint64_t value;
        uint64_t primary_key() const { return id; }
        checksum256 by_scriptpubkey() const { return xsat::utils::hash(scriptpubkey); }
        checksum256 by_outpoint() const { return get_output_id(txid, index); }
    };
    typedef eosio::multi_index<
        "utxos"_n, utxo_row,
        eosio::indexed_by<"scriptpubkey"_n, const_mem_fun<utxo_row, checksum256, &utxo_row::by_scriptpubkey>>,
        eosio::indexed_by<"byoutpoint"_n, const_mem_fun<utxo_row, checksum256, &utxo_row::by_outpoint>>>
        utxo_table;

    /**
     * @brief block table.
     * @scope `get_self()`
     *
     * @field height - the height of the block
     * @field hash - the hash of the block
     * @field cumulative_work - the cumulative workload of the block
     * @field version - block version
     * @field previous_block_hash - hash in internal byte order of the previous block’s header.
     * @field merkle - the merkle root is derived from the hashes of all transactions included in this block
     * @field timestamp - the block time is a Unix epoch time
     * @field bits - An encoded version of the target threshold this block’s header hash must be less than or equal
     * to
     * @field nonce -An arbitrary number miners change to modify the header hash in order to produce a hash less than or
     */
    struct [[eosio::table]] block_row {
        uint64_t height;
        checksum256 hash;
        checksum256 cumulative_work;
        uint32_t version;
        checksum256 previous_block_hash;
        checksum256 merkle;
        uint32_t timestamp;
        uint32_t bits;
        uint32_t nonce;
        uint64_t primary_key() const { return height; }
        checksum256 by_hash() const { return hash; }
    };
    typedef eosio::multi_index<
        "blocks"_n, block_row,
        eosio::indexed_by<"byhash"_n, const_mem_fun<block_row, checksum256, &block_row::by_hash>>>
        block_table;

    /**
     * @brief block extra table.
     * @scope `height`
     *
     * @field bucket_id - the hash of the block
     */
    struct [[eosio::table]] block_extra_row {
        uint16_t bucket_id;
    };
    typedef eosio::singleton<"block.extra"_n, block_extra_row> block_extra_table;

    /**
     * @brief consensus success block table.
     * @scope `get_self()`
     *
     * @field bucket_id - primary key of blockbuckets table
     * @field height - the height of the block
     * @field hash - the hash of the block
     * @field cumulative_work - the cumulative workload of the block
     * @field version - block version
     * @field previous_block_hash - hash in internal byte order of the previous block’s header.
     * @field merkle - the merkle root is derived from the hashes of all transactions included in this block
     * @field timestamp - the block time is a Unix epoch time
     * @field bits - An encoded version of the target threshold this block’s header hash must be less than or equal
     * to
     * @field nonce -An arbitrary number miners change to modify the header hash in order to produce a hash less than or
     * @field miner - block miner address
     * @field synchronizer - block synchronizer address
     * @field num_provider_validators - the number of users who have endorsed the current block
     */
    struct [[eosio::table]] consensus_block_row {
        uint64_t bucket_id;
        uint64_t height;
        checksum256 hash;
        checksum256 cumulative_work;
        uint32_t version;
        checksum256 previous_block_hash;
        checksum256 merkle;
        uint32_t timestamp;
        uint32_t bits;
        uint32_t nonce;
        name miner;
        name synchronizer;
        uint32_t num_provider_validators;
        uint64_t primary_key() const { return bucket_id; }
        uint64_t by_height() const { return height; }
        checksum256 by_block_id() const { return xsat::utils::compute_block_id(height, hash); }
        uint64_t by_synchronizer() const { return synchronizer.value; }
    };
    typedef eosio::multi_index<
        "consensusblk"_n, consensus_block_row,
        eosio::indexed_by<"byheight"_n, const_mem_fun<consensus_block_row, uint64_t, &consensus_block_row::by_height>>,
        eosio::indexed_by<"byblockid"_n,
                          const_mem_fun<consensus_block_row, checksum256, &consensus_block_row::by_block_id>>,
        eosio::indexed_by<"bysyncer"_n,
                          const_mem_fun<consensus_block_row, uint64_t, &consensus_block_row::by_synchronizer>>>
        consensus_block_table;

    struct process_block_result {
        uint64_t height;
        checksum256 block_hash;
        string status;
    };

    /**
     * init chainstate action.
     * @auth `get_self()`
     *
     * @param height - the height of the block
     * @param hash - the hash of the block
     * @param cumulative_work - the cumulative workload of the block
     *
     */
    [[eosio::action]]
    void init(const uint64_t height, const checksum256 &hash, const checksum256 &cumulative_work);

    /**
     * Config action.
     * @auth `get_self()`
     *
     * @param parse_timeout_seconds - parsing timeout duration
     * @param num_validators_per_distribution - number of endorsing users each time rewards are distributed
     * @param num_retain_data_blocks - number of blocks to retain data
     * @param num_merkle_layer - verify the number of merkle levels (log(num_txs_per_verification))
     *
     */
    [[eosio::action]]
    void config(const uint16_t parse_timeout_seconds, const uint16_t num_validators_per_distribution,
                const uint16_t num_retain_data_blocks, const uint8_t num_merkle_layer);

    /**
     * add utxo action.
     * @auth `get_self()`
     *
     * @param id - primary key
     * @param txid - transaction id
     * @param index - vout index
     * @param scriptpubkey - script public key
     * @param value - utxo quantity
     *
     */
    [[eosio::action]]
    void addutxo(const uint64_t id, const checksum256 &txid, const uint32_t index, const vector<uint8_t> &scriptpubkey,
                 const uint64_t value);

    /**
     * delete utxo action.
     * @auth `get_self()`
     *
     * @param id - primary key id for table [utxos]
     *
     */
    [[eosio::action]]
    void delutxo(const uint64_t id);

    /**
     * add block action.
     *
     * @param height - the height of the block
     * @param hash - the hash of the block
     * @param cumulative_work - the cumulative workload of the block
     * @param version - block version
     * @param previous_block_hash - hash in internal byte order of the previous block’s header.
     * @param merkle - the merkle root is derived from the hashes of all transactions included in this block
     * @param timestamp - the block time is a Unix epoch time
     * @param bits - An encoded version of the target threshold this block’s header hash must be less than or equal
     * to
     * @param nonce -An arbitrary number miners change to modify the header hash in order to produce a hash less than or
     */
    [[eosio::action]]
    void addblock(const uint64_t height, const checksum256 &hash, const checksum256 &cumulative_work,
                  const uint32_t version, const checksum256 &previous_block_hash, const checksum256 &merkle,
                  const uint32_t timestamp, const uint32_t bits, const uint32_t nonce);

    /**
     * delete block action.
     * @auth `get_self()`
     *
     * @param height - block height
     *
     */
    [[eosio::action]]
    void delblock(const uint64_t height);

    /**
     * process block action.
     * @auth `synchronizer`
     *
     * @param synchronizer - synchronizer account
     * @param process_rows - Number of VIns and Vouts to be parsed
     *
     */
    [[eosio::action]]
    process_block_result processblock(const name &synchronizer, uint64_t process_rows);

    /**
     * Consensus action.
     * @auth blksync.xsat or blkendt.xsat
     *
     * @param height - Caller account
     * @param hash - Number of VIns and Vouts to be parsed
     *
     */
    [[eosio::action]]
    void consensus(const uint64_t height, const checksum256 &hash);

    using consensus_action = eosio::action_wrapper<"consensus"_n, &utxo_manage::consensus>;

    static std::string get_parsing_status_name(const parsing_status status) {
        switch (status) {
            // Because wait starts from a new block, the return name is parsing completed.
            case waiting:
                return "parsing_completed";
            case parsing:
                return "parsing";
            case deleting_data:
                return "deleting_data";
            case distributing_rewards:
                return "distributing_rewards";
            default:
                return "invalid_status";
        }
    }

    static checksum256 get_output_id(const checksum256 &txid, const uint32_t index) {
        std::vector<char> result;
        result.resize(36);
        eosio::datastream<char *> ds(result.data(), result.size());
        ds << txid;
        ds << index;
        return eosio::sha256((char *)result.data(), result.size());
    }

    static bool check_consensus(const uint64_t height, const eosio::checksum256 &hash) {
        utxo_manage::consensus_block_table _consensus_block(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
        auto consensus_block_idx = _consensus_block.get_index<"byblockid"_n>();
        auto consensus_block_itr = consensus_block_idx.find(xsat::utils::compute_block_id(height, hash));
        if (consensus_block_itr != consensus_block_idx.end()) return true;

        utxo_manage::block_table _block(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
        auto block_itr = _block.find(height);
        return block_itr != _block.end();
    }

    static std::optional<eosio::checksum256> get_cumulative_work(const uint64_t height,
                                                                 const eosio::checksum256 &hash) {
        utxo_manage::consensus_block_table _consensus_block(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
        auto consensus_block_idx = _consensus_block.get_index<"byblockid"_n>();
        auto consensus_block_itr = consensus_block_idx.find(xsat::utils::compute_block_id(height, hash));
        if (consensus_block_itr == consensus_block_idx.end()) {
            utxo_manage::block_table _block(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
            auto block_idx = _block.get_index<"byhash"_n>();
            auto block_itr = block_idx.find(hash);
            if (block_itr == block_idx.end()) {
                return std::nullopt;
            } else {
                return block_itr->cumulative_work;
            }
        } else {
            return consensus_block_itr->cumulative_work;
        }
    }

   private:
    // table init
    config_table _config = config_table(_self, _self.value);
    chain_state_table _chain_state = chain_state_table(_self, _self.value);
    utxo_table _utxo = utxo_table(_self, _self.value);
    block_table _block = block_table(_self, _self.value);
    consensus_block_table _consensus_block = consensus_block_table(_self, _self.value);

    // private function
    consensus_block_row find_next_irreversible_block(const uint64_t irreversible_height,
                                                     const checksum256 &irreversible_hash);
    void find_set_next_block(utxo_manage::chain_state_row *chain_state);

    void process_transactions(utxo_manage::chain_state_row *chain_state, uint64_t process_row);

    void remove_utxo(const checksum256 &prev_txid, const uint32_t prev_index);

    utxo_row save_utxo(const std::vector<uint8_t> &script_data, const uint64_t value, const checksum256 &txid,
                       const uint32_t index);
};
