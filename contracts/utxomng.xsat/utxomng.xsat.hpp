#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include "../internal/defines.hpp"
#include "../internal/utils.hpp"

using namespace eosio;
using namespace std;

class [[eosio::contract("utxomng.xsat")]] utxo_manage : public contract {
   public:
    using contract::contract;

    typedef uint8_t parsing_status;
    static const parsing_status waiting = 1;
    static const parsing_status parsing = 2;
    static const parsing_status deleting_data = 3;
    static const parsing_status distributing_rewards = 4;

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

    /**
     * ## TABLE `chainstate`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} head_height` - header block height for consensus success
     * - `{uint64_t} irreversible_height` - irreversible block height
     * - `{uint64_t} irreversible_hash` - irreversible block hash
     * - `{uint64_t} num_utxos` - total number of UTXOs
     * - `{uint8_t} status` - parsing status @see `parsing_status`
     * - `{uint64_t} parsing_height` - the current height being parsed
     * - `{checksum256} parsing_hash` - the hash currently being parsed
     * - `{uint64_t} parsing_bucket_id` - the bucket_id currently being parsed
     * - `{uint64_t} num_transactions` - the number of transactions currently parsing the block
     * - `{uint64_t} processed_transactions` - number of transactions currently resolved
     * - `{uint64_t} processed_position` - the position of the currently parsed block
     * - `{uint64_t} processed_vin` - the current transaction has been resolved to the vin index
     * - `{uint64_t} processed_vout` - the current transaction has been resolved to the vout index
     * - `{uint32_t} num_provider_validators` - the number of validators that have endorsed the current parsed block
     * - `{uint32_t} num_validators_assigned` - the number of validators that have been allocated rewards
     * - `{name} miner` - block miner account
     * - `{name} synchronizer` - block synchronizer account
     * - `{name} parser` - the account number of the parsing block
     * - `{time_point_sec} parsed_expiration_time` - timeout for parsing chunks
     *
     * ### example
     *
     * ```json
     * {
     *   "head_height": 840043,
     *   "irreversible_height": 840001,
     *   "irreversible_hash": "00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463",
     *   "num_utxos": 15757,
     *   "status": 1,
     *   "parsing_height": 840002,
     *   "parsing_hash": "00000000000000000002c0cc73626b56fb3ee1ce605b0ce125cc4fb58775a0a9",
     *   "parsing_bucket_id": 3,
     *   "num_transactions": 0,
     *   "processed_transactions": 0,
     *   "processed_position": 0,
     *   "processed_vin": 0,
     *   "processed_vout": 0,
     *   "num_provider_validators": 2,
     *   "num_validators_assigned": 0,
     *   "miner": "",
     *   "synchronizer": "alice",
     *   "parser": "alice",
     *   "parsed_expiration_time": "2024-07-13T14:39:32"
     * }
     * ```
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
        time_point_sec parsed_expiration_time;
    };
    typedef eosio::singleton<"chainstate"_n, chain_state_row> chain_state_table;

    /**
     * ## TABLE `config`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint16_t} parse_timeout_seconds` - parsing timeout duration
     * - `{uint16_t} num_validators_per_distribution` - number of endorsing users each time rewards are distributed
     * - `{uint16_t} num_retain_data_blocks` - number of blocks to retain data
     * - `{uint16_t} num_txs_per_verification` - the number of tx for each verification (2^n)
     * - `{uint8_t} num_merkle_layer` - verify the number of merkle levels (log(num_txs_per_verification))
     * - `{uint16_t} num_miner_priority_blocks` - miners who produce blocks give priority to verifying the number of
     * blocks
     *
     * ### example
     *
     * ```json
     * {
     *   "parse_timeout_seconds": 600,
     *   "num_validators_per_distribution": 100,
     *   "num_retain_data_blocks": 100,
     *   "num_txs_per_verification": 1024,
     *   "num_merkle_layer": 10,
     *   "num_miner_priority_blocks": 10
     *  }
     * ```
     */
    struct [[eosio::table("config")]] config_row {
        uint16_t parse_timeout_seconds = 600;
        uint16_t num_validators_per_distribution = 100;
        uint16_t num_retain_data_blocks = 100;
        uint16_t num_txs_per_verification = 2048;
        uint8_t num_merkle_layer = 11;
        uint16_t num_miner_priority_blocks = 10;
    };
    typedef eosio::singleton<"config"_n, config_row> config_table;

    /**
     * ## TABLE `utxos`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} id` - primary key
     * - `{checksum256} txid` - transaction id
     * - `{uint32_t} index` - vout index
     * - `{std::vector<uint8_t>} scriptpubkey` - vout's script public key
     * - `{uint32_t} value` - utxo quantity
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 2,
     *   "txid": "2bb85f4b004be6da54f766c17c1e855187327112c231ef2ff35ebad0ea67c69e",
     *   "index": 0,
     *   "scriptpubkey": "51203b8b3ab1453eb47e2d4903b963776680e30863df3625d3e74292338ae7928da1",
     *   "value": 1797928002
     * }
     * ```
     */
    struct [[eosio::table]] utxo_row {
        uint64_t id;
        checksum256 txid;
        uint32_t index;
        std::vector<uint8_t> scriptpubkey;
        uint64_t value;
        uint64_t primary_key() const { return id; }
        checksum256 by_scriptpubkey() const { return xsat::utils::hash(scriptpubkey); }
        checksum256 by_utxo_id() const { return compute_utxo_id(txid, index); }
    };
    typedef eosio::multi_index<
        "utxos"_n, utxo_row,
        eosio::indexed_by<"scriptpubkey"_n, const_mem_fun<utxo_row, checksum256, &utxo_row::by_scriptpubkey>>,
        eosio::indexed_by<"byutxoid"_n, const_mem_fun<utxo_row, checksum256, &utxo_row::by_utxo_id>>>
        utxo_table;

    /**
     * ## TABLE `blocks`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{checksum256} cumulative_work` - the cumulative workload of the block
     * - `{uint32_t} version` - block version
     * - `{checksum256} previous_block_hash` - hash in internal byte order of the previous block’s header
     * - `{checksum256} merkle` - the merkle root is derived from the hashes of all transactions included in this block
     * - `{uint32_t} timestamp` - the block time is a Unix epoch time
     * - `{uint32_t} bits` - an encoded version of the target threshold this block’s header hash must be less than or
     * equal to
     * - `{uint32_t} nonce` - an arbitrary number miners change to modify the header hash in order to produce a hash
     * less than or
     *
     * ### example
     *
     * ```json
     * {
     *   "height": 840011,
     *   "hash": "00000000000000000002d12efb02bcf70580b2eebf4b775578844640512e30f3",
     *   "cumulative_work": "0000000000000000000000000000000000000000753f3af9322a2a893cb6ece4",
     *   "version": 747323392,
     *   "previous_block_hash": "00000000000000000000da20f7d8e9e6412d4f1d8b62d88264cddbdd48256ba0",
     *   "merkle": "476eac37dd22a59952c5812f715a3e39b68663e40d80d756ba44d7d59e7d6a0a",
     *   "timestamp": 1713576761,
     *   "bits": 386089497,
     *   "nonce": 1492849681,
     *  }
     * ```
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
     * ## TABLE `block.extra`
     *
     * ### scope `height`
     * ### params
     *
     * - `{uint64_t} bucket_id` - the associated bucket number is used to obtain block data
     *
     * ### example
     *
     * ```json
     * {
     *   "bucket_id": 1,
     * }
     * ```
     */
    struct [[eosio::table]] block_extra_row {
        uint64_t bucket_id;
    };
    typedef eosio::singleton<"block.extra"_n, block_extra_row> block_extra_table;

    /**
     * ## TABLE `consensusblk`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} bucket_id` - the associated bucket number is used to obtain block data
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{checksum256} cumulative_work` - the cumulative workload of the block
     * - `{uint32_t} version` - block version
     * - `{checksum256} previous_block_hash` - hash in internal byte order of the previous block’s header
     * - `{checksum256} merkle` - the merkle root is derived from the hashes of all transactions included in this block
     * - `{uint32_t} timestamp` - the block time is a Unix epoch time
     * - `{uint32_t} bits` - an encoded version of the target threshold this block’s header hash must be less than or
     * equal to
     * - `{uint32_t} nonce` - an arbitrary number miners change to modify the header hash in order to produce a hash
     * less than or
     * - `{name} miner` - block miner account
     * - `{name} synchronizer` - block synchronizer account
     *
     * ### example
     *
     * ```json
     * {
     *   "bucket_id": 13,
     *   "height": 840011,
     *   "hash": "00000000000000000002d12efb02bcf70580b2eebf4b775578844640512e30f3",
     *   "cumulative_work": "0000000000000000000000000000000000000000753f3af9322a2a893cb6ece4",
     *   "version": 747323392,
     *   "previous_block_hash": "00000000000000000000da20f7d8e9e6412d4f1d8b62d88264cddbdd48256ba0",
     *   "merkle": "476eac37dd22a59952c5812f715a3e39b68663e40d80d756ba44d7d59e7d6a0a",
     *   "timestamp": 1713576761,
     *   "bits": 386089497,
     *   "nonce": 1492849681,
     *   "miner": "",
     *   "synchronizer": "alice"
     *  }
     * ```
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

    /**
     * ## STRUCT `process_block_result`
     *
     * ### params
     *
     * - `{uint64_t} height` - block height
     * - `{checksum256} block_hash` - block hash
     * - `{string} status` - current parsing status
     *
     * ### example
     *
     * ```json
     * {
     *   "height": 840000,
     *   "block_hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5",
     *   "status": "parsing",
     * }
     * ```
     */
    struct process_block_result {
        uint64_t height;
        checksum256 block_hash;
        string status;
    };

    /**
     * ## ACTION `init`
     *
     * - **authority**: `get_self()`
     *
     * > Initialize block information to start parsing.
     *
     * ### params
     *
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{checksum256} cumulative_work` - the cumulative workload of the block
     *
     * ### example
     *
     * ```bash
     * $ cleos push action utxomng.xsat init '[839999,
     * "0000000000000000000172014ba58d66455762add0512355ad651207918494ab",
     * "0000000000000000000000000000000000000000753b8c1eaae701e1f0146360"]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void init(const uint64_t height, const checksum256 &hash, const checksum256 &cumulative_work);

    /**
     * ## ACTION `config`
     *
     * - **authority**: `get_self()`
     *
     * > Setting parameters.
     *
     * ### params
     *
     * - `{uint16_t} parse_timeout_seconds` - parsing timeout duration
     * - `{uint16_t} num_validators_per_distribution` - number of endorsing users each time rewards are distributed
     * - `{uint16_t} num_retain_data_blocks` - number of blocks to retain data
     * - `{uint16_t} num_txs_per_verification` - the number of tx for each verification (2^n)
     * - `{uint8_t} num_merkle_layer` - verify the number of merkle levels (log(num_txs_per_verification))
     * - `{uint16_t} num_miner_priority_blocks` - miners who produce blocks give priority to verifying the number of
     * blocks
     *
     * ### example
     *
     * ```bash
     * $ cleos push action utxomng.xsat config '[600, 100, 100, 11, 10]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void config(const uint16_t parse_timeout_seconds, const uint16_t num_validators_per_distribution,
                const uint16_t num_retain_data_blocks, const uint8_t num_merkle_layer,
                const uint16_t num_miner_priority_blocks);

    /**
     * ## ACTION `addutxo`
     *
     * - **authority**: `get_self()`
     *
     * > Add utxo data.
     *
     * ### params
     *
     * - `{uint64_t} id` - primary id
     * - `{checksum256} txid` - transaction id
     * - `{uint32_t} index` - vout index
     * - `{vector<uint8_t>} scriptpubkey` - script public key
     * - `{uint64_t} value` - utxo quantity
     *
     * ### example
     *
     * ```bash
     * $ cleos push action utxomng.xsat addutxo '[1, "c323eae524ce3b49f0868396eb9a61bea0e5fb3dc2e52cb46e04c2dba28a3c0d",
     * 1, "001435f6de260c9f3bdee47524c473a6016c0c055cb9", 636813647]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void addutxo(const uint64_t id, const checksum256 &txid, const uint32_t index, const vector<uint8_t> &scriptpubkey,
                 const uint64_t value);

    /**
     * ## ACTION `delutxo`
     *
     * - **authority**: `get_self()`
     *
     * > Delete utxo data.
     *
     * ### params
     *
     * - `{uint64_t} id` - utxo id
     *
     * ### example
     *
     * ```bash
     * $ cleos push action utxomng.xsat delutxo '[1]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void delutxo(const uint64_t id);

    /**
     * ## ACTION `addblock`
     *
     * - **authority**: `get_self()`
     *
     * > Add history block header.
     *
     * ### params
     *
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{checksum256} cumulative_work` - the cumulative workload of the block
     * - `{uint32_t} version` - block version
     * - `{checksum256} previous_block_hash` - hash in internal byte order of the previous block’s header
     * - `{checksum256} merkle` - the merkle root is derived from the hashes of all transactions included in this block
     * - `{uint32_t} timestamp` - the block time is a Unix epoch time
     * - `{uint32_t} bits` - an encoded version of the target threshold this block’s header hash must be less than or
     * equal to
     * - `{uint32_t} nonce` - an arbitrary number miners change to modify the header hash in order to produce a hash
     * less than or
     *
     * ### example
     *
     * ```bash
     * $ cleos push action utxomng.xsat addblock '[840000,
     * "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5",
     * "0000000000000000000172014ba58d66455762add0512355ad651207918494ab", 710926336,
     * "0000000000000000000172014ba58d66455762add0512355ad651207918494ab",
     * "031b417c3a1828ddf3d6527fc210daafcc9218e81f98257f88d4d43bd7a5894f", 1713571767, 3932395645, 386089497
     * ]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void addblock(const uint64_t height, const checksum256 &hash, const checksum256 &cumulative_work,
                  const uint32_t version, const checksum256 &previous_block_hash, const checksum256 &merkle,
                  const uint32_t timestamp, const uint32_t bits, const uint32_t nonce);

    /**
     * ## ACTION `delblock`
     *
     * - **authority**: `get_self()`
     *
     * > Delete block header.
     *
     * ### params
     *
     * - `{uint64_t} height` - block height
     *
     * ### example
     *
     * ```bash
     * $ cleos push action utxomng.xsat delblock '[840000]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void delblock(const uint64_t height);

    /**
     * ## ACTION `processblock`
     *
     * - **authority**: `synchronizer`
     *
     * > Parse utxo
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} process_rows` - number of vins and vouts to be parsed
     *
     * ### example
     *
     * ```bash
     * $ cleos push action utxomng.xsat processblock '["alice", 1000]' -p alice
     * ```
     */
    [[eosio::action]]
    process_block_result processblock(const name &synchronizer, uint64_t process_rows);

    /**
     * ## ACTION `consensus`
     *
     * - **authority**: `blksync.xsat` or `blkendt.xsat`
     *
     * > Reach consensus logic
     *
     * ### params
     *
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     *
     * ### example
     *
     * ```bash
     * $ cleos push action utxomng.xsat consensus '[840000,
     * "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"]' -p blksync.xsat
     * ```
     */
    [[eosio::action]]
    void consensus(const uint64_t height, const checksum256 &hash);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<uint64_t> scope, const optional<uint64_t> max_rows);
#endif

    using consensus_action = eosio::action_wrapper<"consensus"_n, &utxo_manage::consensus>;

    static checksum256 compute_utxo_id(const checksum256 &tx_id, const uint32_t index) {
        std::vector<char> result;
        result.resize(36);
        eosio::datastream<char *> ds(result.data(), result.size());
        ds << tx_id;
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

#ifdef DEBUG
    template <typename T>
    void clear_table(T &table, uint64_t rows_to_clear);
#endif
};
