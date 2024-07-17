#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include "../internal/utils.hpp"

using namespace eosio;
using namespace std;

class [[eosio::contract("staking.xsat")]] stake : public contract {
   public:
    using contract::contract;

    /**
     * ## TABLE `globalid`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} staking_id` - the latest staking id
     * - `{uint64_t} release_id` - the latest release id
     *
     * ### example
     *
     * ```json
     * {
     *   "staking_id": 1,
     *   "release_id": 1
     * }
     * ```
     */
    struct [[eosio::table()]] global_id_row {
        uint64_t staking_id;
        uint64_t release_id;
    };
    typedef eosio::singleton<"globalid"_n, global_id_row> global_id_table;

    /**
     * ## TABLE `tokens`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} id` - token id
     * - `{uint64_t} token` - whitelist token
     * - `{bool} disabled_staking` - whether to disable staking
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 1,
     *   "token": { "sym": "8,BTC", "contract": "btc.xsat" },
     *   "disabled_staking": false
     * }
     * ```
     */
    struct [[eosio::table]] token_row {
        uint64_t id;
        extended_symbol token;
        bool disabled_staking;
        uint64_t primary_key() const { return id; }
        uint128_t by_token() const { return xsat::utils::compute_id(token); }
    };
    typedef eosio::multi_index<"tokens"_n, token_row,
                               indexed_by<"bytoken"_n, const_mem_fun<token_row, uint128_t, &token_row::by_token>>>
        token_table;

    /**
     * ## TABLE `staking`
     *
     * ### scope `staker`
     * ### params
     *
     * - `{uint64_t} id` - staking id
     * - `{extended_asset} quantity` - total number of staking
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 1,
     *   "quantity": {"quantity":"1.00000000 BTC", "contract":"btc.xsat"}
     * }
     * ```
     */
    struct [[eosio::table]] staking_row {
        uint64_t id;
        extended_asset quantity;
        uint64_t primary_key() const { return id; }
        uint128_t by_token() const { return xsat::utils::compute_id(quantity.get_extended_symbol()); }
    };
    typedef eosio::multi_index<"staking"_n, staking_row,
                               indexed_by<"bytoken"_n, const_mem_fun<staking_row, uint128_t, &staking_row::by_token>>>
        staking_table;

    /**
     * ## TABLE `releases`
     *
     * ### scope `staker`
     * ### params
     *
     * - `{uint64_t} id` - release id
     * - `{extended_asset} quantity` - unpledged quantity
     * - `{time_point_sec} expiration_time` - cancel pledge expiration time
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 1,
     *   "quantity": {
     *       "quantity": "1.00000000 BTC",
     *       "contract": "btc.xsat"
     *   },
     *   "expiration_time": "2024-08-12T08:09:57"
     * }
     * ```
     */
    struct [[eosio::table]] release_row {
        uint64_t id;
        extended_asset quantity;
        time_point_sec expiration_time;
        uint64_t primary_key() const { return id; }
        uint64_t by_expiration_time() const { return expiration_time.sec_since_epoch(); }
    };
    typedef eosio::multi_index<
        "releases"_n, release_row,
        eosio::indexed_by<"byexpire"_n, const_mem_fun<release_row, uint64_t, &release_row::by_expiration_time>>>
        release_table;

    /**
     * ## ACTION `addtoken`
     *
     * - **authority**: `get_self()`
     *
     * > Add whitelist token
     *
     * ### params
     *
     * - `{extended_symbol} token` - token to add
     *
     * ### example
     *
     * ```bash
     * $ cleos push action staking.xsat addtoken '[{ "sym": "8,BTC", "contract": "btc.xsat" }]' -p staking.xsat
     * ```
     */
    [[eosio::action]]
    void addtoken(const extended_symbol& token);

    /**
     * ## ACTION `deltoken`
     *
     * - **authority**: `get_self()`
     *
     * > Delete whitelist token
     *
     * ### params
     *
     * - `{uint64_t} id` - token id to be deleted
     *
     * ### example
     *
     * ```bash
     * $ cleos push action staking.xsat deltoken '[1]' -p staking.xsat
     * ```
     */
    [[eosio::action]]
    void deltoken(const uint64_t id);

    /**
     * ## ACTION `setstatus`
     *
     * - **authority**: `get_self()`
     *
     * > Set the token’s disabled staking status.
     *
     * ### params
     *
     * - `{uint64_t} id` - token id
     * - `{bool} disabled_staking` - whether to disable staking
     *
     * ### example
     *
     * ```bash
     * $ cleos push action staking.xsat setstatus '[1, true]' -p staking.xsat
     * ```
     */
    [[eosio::action]]
    void setstatus(const uint64_t id, const bool disabled_staking);

    /**
     * ## ACTION `release`
     *
     * - **authority**: `staker`
     *
     * > Cancel the pledge and enter the unlocking period.
     *
     * ### params
     *
     * - `{uint64_t} staking_id` - staking id
     * - `{name} staker` - staker account
     * - `{name} validator` - the validator account to be pledged to
     * - `{extended_asset} quantity` - unpledged quantity
     *
     * ### example
     *
     * ```bash
     * $ cleos push action staking.xsat release '[1, "alice", "alice", "1.00000000 BTC"]' -p alice
     * ```
     */
    [[eosio::action]]
    void release(const uint64_t staking_id, const name& staker, const name& validator, const asset& quantity);

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
    token_table _token = token_table(_self, _self.value);

    uint64_t next_release_id();
    uint64_t next_staking_id();
    asset get_balance(const name& owner, const extended_symbol& token);
    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);
    void do_stake(const name& from, const name& validator, const extended_asset& ext_in);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};