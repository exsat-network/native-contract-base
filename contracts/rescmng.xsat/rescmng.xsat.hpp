#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "../internal/defines.hpp"

using namespace eosio;
using std::string;

class [[eosio::contract("rescmng.xsat")]] resource_management : public contract {
   public:
    using contract::contract;

    /**
     * @brief config table.
     * @scope `get_self()`
     *
     * @field fee_account - account number for receiving handling fees
     * @field disabled_withdraw - whether withdrawal of balance is allowed
     * @field cost_per_slot - cost per slot
     * @field cost_per_upload - cost per upload chunk
     * @field cost_per_verification - the cost of each verification performed
     * @field cost_per_endorsement - the cost of each execution of an endorsement
     * @field cost_per_parse - cost per execution of parsing
     *
     **/
    struct [[eosio::table]] config_row {
        name fee_account;
        bool disabled_withdraw = false;
        asset cost_per_slot = {0, BTC_SYMBOL};
        asset cost_per_upload = {0, BTC_SYMBOL};
        asset cost_per_verification = {0, BTC_SYMBOL};
        asset cost_per_endorsement = {0, BTC_SYMBOL};
        asset cost_per_parse = {0, BTC_SYMBOL};
    };
    typedef eosio::singleton<"config"_n, config_row> config_table;

    /**
     * @brief account balance table.
     * @scope `get_self()`
     *
     * @field owner - primary key
     * @field balance - account balance
     *
     **/
    struct [[eosio::table]] account_row {
        name owner;
        asset balance;
        uint64_t primary_key() const { return owner.value; }
    };
    typedef eosio::multi_index<"accounts"_n, account_row> account_table;

    /**
     * Update config action.
     * @auth `get_self()`
     *
     * @param fee_account - account number for receiving handling fees
     * @param cost_per_slot - cost per slot
     * @param cost_per_upload - cost per upload chunk
     * @param cost_per_verification - the cost of each verification performed
     * @param cost_per_endorsement - the cost of each execution of an endorsement
     * @param cost_per_parse - cost per execution of parsing
     *
     */
    [[eosio::action]]
    void init(const name& fee_account, const asset& cost_per_slot, const asset& cost_per_upload,
              const asset& cost_per_verification, const asset& cost_per_endorsement, const asset& cost_per_parse);
    /**
     * Deduct user balance action.
     * @auth `blksync.xsat` or `blkendt.xsat` or `utxomng.xsat` or `poolreg.xsat`
     *
     * @param height - block height
     * @param hash - block hash
     * @param owner - debit account
     * @param type - type of deduction
     * @param quantity - deduction quantity
     *
     */
    [[eosio::action]]
    void pay(const uint64_t height, const checksum256& hash, const name& owner, const fee_type type,
             const uint64_t quantity);

    /**
     * Withdraw balance action.
     * @auth `owner`
     *
     * @param owner - account for withdrawing fees
     * @param quantity - the number of tokens to be withdrawn
     *
     */
    [[eosio::action]]
    void withdraw(const name& owner, const asset& quantity);

    /**
     * Setting withdraw status action.
     * @auth `get_self()`
     *
     * @param disabled_withdraw - whether to disable withdrawal of balance
     *
     */
    [[eosio::action]]
    void setstatus(const bool disabled_withdraw);

    [[eosio::on_notify("*::transfer")]]
    void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    // logs
    [[eosio::action]]
    void depositlog(const name& owner, const asset& quantity, const asset& balance) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void withdrawlog(const name& owner, const asset& quantity, const asset& balance) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void paylog(const uint64_t height, const checksum256& hash, const name& owner, const uint8_t& type,
                const asset& quantity) {
        require_auth(get_self());
    }

    using pay_action = eosio::action_wrapper<"pay"_n, &resource_management::pay>;
    using depositlog_action = eosio::action_wrapper<"depositlog"_n, &resource_management::depositlog>;
    using withdrawlog_action = eosio::action_wrapper<"withdrawlog"_n, &resource_management::withdrawlog>;
    using paylog_action = eosio::action_wrapper<"paylog"_n, &resource_management::paylog>;

   private:
    // table init
    account_table _account = account_table(_self, _self.value);
    config_table _config = config_table(_self, _self.value);

    // private method
    void do_deposit(const name& from, const name& contract, const asset& quantity, const string& memo);
    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);
    asset get_fee(const fee_type type, const uint64_t time_or_size);
};