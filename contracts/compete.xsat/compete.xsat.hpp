#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("compete.xsat")]] compete : public contract {
   public:
    using contract::contract;

    static constexpr name ENDORSER_MANAGE_CONTRACT = "endrmng.xsat"_n;

    [[eosio::action]]
    void setmindonate(const uint16_t min_donate_rate);

    [[eosio::action]]
    void addround(const uint8_t quota, const optional<time_point_sec> start_time);

    [[eosio::action]]
    void activate(const name& validator);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
#endif

   private:
    /**
     * ## TABLE `globals`
     *
     * ### scope `get_global()`
     * ### params
     *
     * - `{uint64_t} round_id` - the current round identifier
     * - `{uint16_t} min_donate_rate` - the minimum donate rate for a validator
     * - `{uint16_t} total_quotas` - the total number of quotas available for all rounds
     * - `{uint64_t} total_activations` - the total number of activations that have occurred
     *
     * ### example
     *
     * ```
     * {
     *   "round_id": 1,
     *   "min_donate_rate": 2000,
     *   "total_quotas": 7,
     *   "total_activations": 2
     * }
     * ```
     */
    struct [[eosio::table]] global_row {
        uint64_t round_id;
        uint16_t min_donate_rate;
        uint16_t total_quotas;
        uint64_t total_activations;
    };
    typedef eosio::singleton<"globals"_n, global_row> global_table;
    global_table _global = global_table(_self, _self.value);

    /**
     * ## TABLE `rounds`
     *
     * ### scope `get_rounds()`
     * ### params
     *
     * - `{uint64_t} round` - the unique identifier for the round
     * - `{uint16_t} quota` - the number of available quotas for this round
     * - `{time_point_sec} start_time` - the start time of the round
     *
     * ### example
     *
     * ```
     * {
     *   "round": 1,
     *   "quota": 2,
     *   "start_time": "2024-10-21T00:00:00"
     * }
     * ```
     */
    struct [[eosio::table]] round_row {
        uint64_t round;
        uint16_t quota;
        time_point_sec start_time;
        uint64_t primary_key() const { return round; }
    };
    typedef eosio::multi_index<"rounds"_n, round_row> round_table;

    /**
     * ## TABLE `activations`
     *
     * ### scope `get_activations()`
     * ### params
     *
     * - `{name} validator` - the validator account associated with the activation
     * - `{uint64_t} round` - the round identifier for this activation
     * - `{time_point_sec} activation_time` - the time when the validator was activated
     *
     * ### example
     *
     * ```
     * {
     *   "validator": "vali1.sat",
     *   "round": 1,
     *   "activation_time": "2024-10-21T00:00:00"
     * }
     * ```
     */
    struct [[eosio::table]] activation_row {
        name validator;
        uint64_t round;
        time_point_sec activation_time;
        uint64_t primary_key() const { return validator.value; }
        uint64_t by_round() const { return round; }
    };
    typedef eosio::multi_index<"activations"_n, activation_row> activation_table;

    struct [[eosio::table]] validator_row {
        name owner;
        name reward_recipient;
        string memo;
        uint16_t commission_rate;
        asset quantity;
        asset qualification;
        asset xsat_quantity;
        uint16_t donate_rate;
        asset total_donated;
        uint128_t stake_acc_per_share;
        uint128_t consensus_acc_per_share;
        asset staking_reward_unclaimed;
        asset staking_reward_claimed;
        asset consensus_reward_unclaimed;
        asset consensus_reward_claimed;
        asset total_consensus_reward;
        asset consensus_reward_balance;
        asset total_staking_reward;
        asset staking_reward_balance;
        time_point_sec latest_staking_time;
        uint64_t latest_reward_block;
        time_point_sec latest_reward_time;
        bool disabled_staking;
        uint64_t primary_key() const { return owner.value; }
        uint64_t by_btc_total_staking() const { return quantity.amount; }
        uint64_t by_xsat_total_staking() const { return xsat_quantity.amount; }
        uint64_t by_qualification() const { return qualification.amount; }
        uint64_t by_disabled_staking() const { return disabled_staking; }
        uint64_t by_total_donated() const { return total_donated.amount; }
    };
    typedef eosio::multi_index<"validators"_n, validator_row,
                               eosio::indexed_by<"bystakedbtc"_n, const_mem_fun<validator_row, uint64_t, &validator_row::by_btc_total_staking>>,
                               eosio::indexed_by<"bystakedxsat"_n, const_mem_fun<validator_row, uint64_t, &validator_row::by_xsat_total_staking>>,
                               eosio::indexed_by<"byqualifictn"_n, const_mem_fun<validator_row, uint64_t, &validator_row::by_qualification>>,
                               eosio::indexed_by<"bydonate"_n, const_mem_fun<validator_row, uint64_t, &validator_row::by_total_donated>>>
        validator_table;

    // table init
    round_table _round = round_table(_self, _self.value);
    activation_table _activation = activation_table(_self, _self.value);

    uint64_t next_round_id();

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};