#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include "../internal/defines.hpp"
#include "../internal/utils.hpp"

using namespace eosio;
using namespace std;

class [[eosio::contract("blkendt.xsat")]] block_endorse : public contract {
   public:
    using contract::contract;

    /**
     * ## STRUCT `validator_info`
     *
     * - `{name} account` - validator account
     * - `{uint64_t} staking` - the number of validator pledges
     *
     * ### example
     *
     * ```json
     * {
     *   "account": "test.xsat",
     *   "staking": "10200000000"
     * }
     * ```
     */
    struct validator_info {
        name account;
        uint64_t staking;
    };

    /**
     * ## TABLE `endorsements`
     *
     * ### scope `height`
     * ### params
     *
     * - `{uint64_t} id` - primary key
     * - `{checksum256} hash` - endorsement block hash
     * - `{std::vector<validator_info>} requested_validators` - list of unendorsed validators
     * - `{std::vector<validator_info>} provider_validators` - list of endorsed validators
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 0,
     *   "hash": "00000000000000000000da20f7d8e9e6412d4f1d8b62d88264cddbdd48256ba0",
     *   "requested_validators": [],
     *   "provider_validators": [{
     *       "account": "test.xsat",
     *       "staking": "10200000000"
     *      }
     *   ]
     * }
     * ```
     */
    struct [[eosio::table]] endorsement_row {
        uint64_t id;
        checksum256 hash;
        std::vector<validator_info> requested_validators;
        std::vector<validator_info> provider_validators;
        uint64_t primary_key() const { return id; }
        checksum256 by_hash() const { return hash; }

        uint16_t num_validators() const { return requested_validators.size() + provider_validators.size(); }

        uint64_t num_reached_consensus() const {
            return xsat::utils::num_reached_consensus(requested_validators.size() + provider_validators.size());
        }

        bool reached_consensus() const {
            return provider_validators.size() > 0 && provider_validators.size() >= num_reached_consensus();
        }
    };
    typedef eosio::multi_index<
        "endorsements"_n, endorsement_row,
        eosio::indexed_by<"byhash"_n, const_mem_fun<endorsement_row, checksum256, &endorsement_row::by_hash>>>
        endorsement_table;

    /**
     * ## ACTION `endorse`
     *
     * - **authority**: `validator`
     *
     * > Endorsement block
     *
     * ### params
     *
     * - `{name} validator` - validator account
     * - `{uint64_t} height` - to endorse the height of the block
     * - `{checksum256} hash` - to endorse the hash of the block
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blkendt.xsat endorse '["alice", 840000,
     * "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"]' -p alice
     * ```
     */
    [[eosio::action]]
    void endorse(const name& validator, const uint64_t height, const checksum256& hash);

    /**
     * ## ACTION `erase`
     *
     * - **authority**: `utxomng.xsat`
     *
     * > To erase high endorsements
     *
     * ### params
     *
     * - `{uint64_t} height` - to endorse the height of the block
     *
     * ### example
     *
     * ```bash
     * $ cleos push action blkendt.xsat erase '[840000]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void erase(const uint64_t height);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<uint64_t> scope, const optional<uint64_t> max_rows);
#endif

    using erase_action = eosio::action_wrapper<"erase"_n, &block_endorse::erase>;

   private:
    std::vector<validator_info> get_valid_validator();

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};
