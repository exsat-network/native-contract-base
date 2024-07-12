#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include "../internal/defines.hpp"
#include "../internal/utils.hpp"

using namespace eosio;
using std::string;

class [[eosio::contract("blkendt.xsat")]] block_endorse : public contract {
   public:
    using contract::contract;

    struct validator_info {
        name account;
        uint64_t staking;
    };

    /**
     * @brief endorsement table.
     * @scope `height`
     *
     * @field id - primary id
     * @field hash - endorsement block hash
     * @field requested_validators - list of unendorsed validators
     * @field provider_validators - list of endorsed validators
     * @field endorsed_stake - endorsed pledge amount
     * @field reached_consensus_stake - reach consensus pledge amount
     *
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
     * Endorse block action.
     * @auth `validator`
     *
     * @param validator - validator account
     * @param height - to endorse the height of the block
     * @param hash - to endorse the hash of the block
     *
     */
    [[eosio::action]]
    void endorse(const name& validator, const uint64_t height, const checksum256& hash);

    /**
     * erase endorsement action.
     * @auth `utxomng.xsat`
     *
     * @param height - endorsement block height
     *
     */
    [[eosio::action]]
    void erase(const uint64_t height);

    using erase_action = eosio::action_wrapper<"erase"_n, &block_endorse::erase>;

   private:
    std::vector<validator_info> get_valid_validator();
};
