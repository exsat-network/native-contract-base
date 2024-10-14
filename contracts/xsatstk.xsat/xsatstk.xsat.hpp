#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include "../internal/utils.hpp"

using namespace eosio;
using namespace std;

class [[eosio::contract("xsatstk.xsat")]] xsat_stake : public contract {
   public:
    using contract::contract;

    /**
     * ## TABLE `globalid`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} release_id` - the latest release id
     *
     * ### example
     *
     * ```json
     * {
     *   "release_id": 1
     * }
     * ```
     */
    struct [[eosio::table]] global_id_row {
        uint64_t release_id;
    };
    typedef eosio::singleton<"globalid"_n, global_id_row> global_id_table;

    /**
     * ## TABLE `config`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{bool} disabled_staking` - the staking status of xsat,  `true` to disable pledge
     *
     * ### example
     *
     * ```json
     * {
     *   "disabled_staking": true
     * }
     * ```
     */
    struct [[eosio::table]] config_row {
        bool disabled_staking;
    };
    typedef eosio::singleton<"config"_n, config_row> config_table;

    /**
     * ## TABLE `staking`
     *
     * ### scope `staker`
     * ### params
     *
     * - `{name} staker` - the staker account
     * - `{extended_asset} quantity` - total number of staking
     *
     * ### example
     *
     * ```json
     * {
     *   "staker": "alice",
     *   "quantity":"1.00000000 XSAT"
     * }
     * ```
     */
    struct [[eosio::table]] staking_row {
        name staker;
        asset quantity;
        uint64_t primary_key() const { return staker.value; }
    };
    typedef eosio::multi_index<"staking"_n, staking_row> staking_table;

    /**
     * ## TABLE `releases`
     *
     * ### scope `staker`
     * ### params
     *
     * - `{uint64_t} id` - release id
     * - `{asset} quantity` - unpledged quantity
     * - `{time_point_sec} expiration_time` - cancel pledge expiration time
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 1,
     *   "quantity": "1.00000000 XSAT",
     *   "expiration_time": "2024-08-12T08:09:57"
     * }
     * ```
     */
    struct [[eosio::table]] release_row {
        uint64_t id;
        asset quantity;
        time_point_sec expiration_time;
        uint64_t primary_key() const { return id; }
        uint64_t by_expiration_time() const { return expiration_time.sec_since_epoch(); }
    };
    typedef eosio::multi_index<
        "releases"_n, release_row,
        eosio::indexed_by<"byexpire"_n, const_mem_fun<release_row, uint64_t, &release_row::by_expiration_time>>>
        release_table;

    /**
     * ## ACTION `setstatus`
     *
     * - **authority**: `get_self()`
     *
     * > Set the staking status of xsat
     *
     * ### params
     *
     * - `{bool} disabled_staking` - whether to disable staking
     *
     * ### example
     *
     * ```bash
     * $ cleos push action xsatstk.xsat setstatus '[true]' -p xsatstk.xsat
     * ```
     */
    [[eosio::action]]
    void setstatus(const bool disabled_staking);

    /**
     * ## ACTION `release`
     *
     * - **authority**: `staker`
     *
     * > Cancel the pledge and enter the unlocking period.
     *
     * ### params
     *
     * - `{name} staker` - staker account
     * - `{name} validator` - the validator account to be pledged to
     * - `{extended_asset} quantity` - unpledged quantity
     *
     * ### example
     *
     * ```bash
     * $ cleos push action staking.xsat release '["alice", "alice", "1.00000000 XSAT"]' -p alice
     * ```
     */
    [[eosio::action]]
    void release(const name& staker, const name& validator, const asset& quantity);

    /**
     * ## ACTION `withdraw`
     *
     * - **authority**: `staker`
     *
     * > Withdraw expired staking tokens.
     *
     * ### params
     *
     * - `{name} staker` - staker account
     *
     * ### example
     *
     * ```bash
     * $ cleos push action staking.xsat withdraw '["alice"]' -p alice
     * ```
     */
    [[eosio::action]]
    void withdraw(const name& staker);

    [[eosio::on_notify("*::transfer")]]
    void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
#endif

   private:
    // table init
    global_id_table _global_id = global_id_table(_self, _self.value);
    config_table _config = config_table(_self, _self.value);
    staking_table _staking = staking_table(_self, _self.value);

    uint64_t next_release_id();
    asset get_balance(const name& owner, const extended_symbol& token);
    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);
    void do_stake(const name& from, const name& validator, const asset& quantity);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};