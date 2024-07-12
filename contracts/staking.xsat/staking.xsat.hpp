#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include "../internal/utils.hpp"

using namespace eosio;
using std::string;

class [[eosio::contract("staking.xsat")]] stake : public contract {
   public:
    using contract::contract;

    /**
     * @brief global id table.
     * @scope `get_self()`
     *
     * @field staking_id - stake number
     * @field release_id - the number of the release pledge
     */
    struct [[eosio::table()]] global_id_row {
        uint64_t staking_id;
        uint64_t release_id;
    };
    typedef eosio::singleton<"globalid"_n, global_id_row> global_id_table;

    /**
     * @brief whitelist token table.
     * @scope `get_self()`
     *
     * @field id - primary key
     * @field token - whitelist token
     *
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
     * @brief staking table.
     * @scope `staker`
     *
     * @field id - primary key
     * @field quantity - total number of stakes
     *
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
     * @brief  release table.
     * @scope `staker`
     *
     * @field id - primary key
     * @field quantity - the amount to release the token
     * @field expiration_time - unstake expiration time
     *
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
     * add whitelist token action.
     * @auth `get_self()`
     *
     * @param token - token to be added to the whitelist
     *
     */
    [[eosio::action]]
    void addtoken(const extended_symbol& token);

    /**
     * delete whitelist token action.
     * @auth `get_self()`
     *
     * @param id - token id
     *
     */
    [[eosio::action]]
    void deltoken(const uint64_t id);

    /**
     * Setting token status action.
     * @auth `get_self()`
     *
     * @param id - token id
     * @param disabled_staking - token status, if true, staking is prohibited
     *
     */
    [[eosio::action]]
    void setstatus(const uint64_t id, const bool disabled_staking);

    /**
     * Release action.
     * @auth `staker`
     *
     * @param staking_id - stake id
     * @param staker - pledge account
     * @param validator - validator account
     * @param quantity - pledge quantity
     *
     */
    [[eosio::action]]
    void release(const uint64_t staking_id, const name& staker, const name& validator, const asset& quantity);

    /**
     * Withdraw action.
     * @auth `staker`
     *
     * @param staker - account to release the staked token
     *
     */
    [[eosio::action]]
    void withdraw(const name& staker);

    [[eosio::on_notify("*::transfer")]]
    void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

   private:
    // table init
    global_id_table _global_id = global_id_table(_self, _self.value);
    token_table _token = token_table(_self, _self.value);

    uint64_t next_release_id();
    uint64_t next_staking_id();
    asset get_balance(const name& owner, const extended_symbol& token);
    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);
    void do_stake(const name& from, const name& validator, const extended_asset& ext_in);
};