#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "../internal/defines.hpp"
#include <endrmng.xsat/endrmng.xsat.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("rwddist.xsat")]] reward_distribution : public contract {
   public:
    using contract::contract;

    struct validator_info {
        name account;
        uint64_t staking;
    };
    /**
     * @brief reward log table.
     * @scope `get_self()`
     *
     * @field height - block height
     * @field hash - block hash
     * @field synchronizer_rewards - the synchronizer assigns the number of rewards
     * @field consensus_rewards - the consensus validator allocates the number of rewards
     * @field staking_rewards - the validator assigns the number of rewards
     * @field num_validators - the number of validators who pledge more than 100 BTC
     * @field provider_validators - validators who can receive rewards
     * @field endorsed_staking - total endorsed pledge amount
     * @field reached_consensus_staking - the total pledge amount to reach consensus is (number of validators * 2/3 + 1
     *pledge amount)
     * @field num_validators_assigned - the number of validators that have been allocated rewards
     * @field synchronizer - synchronizer address
     * @field miner - miner address
     * @field parser - parse the address of the block
     * @field tx_id - transaction id
     * @field latest_exec_time - latest reward distribution time
     *
     **/
    struct [[eosio::table]] reward_log_row {
        uint64_t height;
        checksum256 hash;
        asset synchronizer_rewards;
        asset consensus_rewards;
        asset staking_rewards;
        uint32_t num_validators;
        std::vector<validator_info> provider_validators;
        uint64_t endorsed_staking;
        uint64_t reached_consensus_staking;
        uint32_t num_validators_assigned;
        name synchronizer;
        name miner;
        name parser;
        checksum256 tx_id;
        time_point_sec latest_exec_time;
        uint64_t primary_key() const { return height; }
    };
    typedef eosio::multi_index<"rewardlogs"_n, reward_log_row> reward_log_table;

    /**
     * Distribute action.
     * @auth `utxomng.xsat`
     *
     * @param height - block height
     * @param hash - block hash
     *
     */
    [[eosio::action]]
    void distribute(const uint64_t height);

    /**
     * Endorser reward action.
     * @auth `utxomng.xsat`
     *
     * @param parser - parse account
     * @param height - block height
     * @param from_index - the starting validator index
     * @param to_index - the ending validator index
     *
     */
    [[eosio::action]]
    void endtreward(const name& parser, const uint64_t height, uint32_t from_index, const uint32_t to_index);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
#endif

    // logs
    [[eosio::action]]
    void rewardlog(const uint64_t height, const checksum256& hash, const name& parser, const asset& synchronizer_reward,
                   const asset& staking_reward, const asset& consensus_reward) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void endtrwdlog(const uint64_t height, const checksum256& hash,
                    const vector<endorse_manage::reward_details_row>& reward_details) {
        require_auth(get_self());
    }

    using distribute_action = eosio::action_wrapper<"distribute"_n, &reward_distribution::distribute>;
    using endtreward_action = eosio::action_wrapper<"endtreward"_n, &reward_distribution::endtreward>;
    using rewardlog_action = eosio::action_wrapper<"rewardlog"_n, &reward_distribution::rewardlog>;
    using endtrwdlog_action = eosio::action_wrapper<"endtrwdlog"_n, &reward_distribution::endtrwdlog>;

   private:
    // init table
    reward_log_table _reward_log = reward_log_table(_self, _self.value);

    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};