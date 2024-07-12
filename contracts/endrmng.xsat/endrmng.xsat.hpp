#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include "../internal/defines.hpp"
#include "../internal/utils.hpp"

using namespace eosio;
using std::string;

class [[eosio::contract("endrmng.xsat")]] endorse_manage : public contract {
   public:
    using contract::contract;

    // CONSTANTS
    const std::set<name> WHITELIST_TYPES = {"proxyreg"_n, "evmcaller"_n};

    /**
     * @brief global id table.
     * @scope `get_self()`
     *
     * @field staking_id - stake number
     */
    struct [[eosio::table]] global_id_row {
        uint64_t staking_id;
    };
    typedef singleton<"globalid"_n, global_id_row> global_id_table;

    /**
     * @brief whitelist table.
     * @scope `get_self()`
     *
     * @field account - whitelist account
     */
    struct [[eosio::table]] whitelist_row {
        name account;
        uint64_t primary_key() const { return account.value; }
    };
    typedef eosio::multi_index<"whitelist"_n, whitelist_row> whitelist_table;

    /**
     * @brief evm proxy table.
     * @scope `get_self()`
     *
     * @field id - primary id
     * @field proxy - evm proxy account
     */
    struct [[eosio::table]] evm_proxy_row {
        uint64_t id;
        checksum160 proxy;
        uint64_t primary_key() const { return id; }
        checksum256 by_proxy() const { return xsat::utils::compute_id(proxy); }
    };
    typedef eosio::multi_index<
        "evmproxys"_n, evm_proxy_row,
        eosio::indexed_by<"byproxy"_n, const_mem_fun<evm_proxy_row, checksum256, &evm_proxy_row::by_proxy>>>
        evm_proxy_table;

    /**
     * @brief evm stake table.
     * @scope `get_self()`
     *
     * @field id - primary key
     * @field proxy - proxy address
     * @field staker - staker address
     * @field validator - validator address
     * @field quantity - total number of staking
     * @field stake_debt - amount of requested stake debt
     * @field staking_reward_unclaimed - amount of stake unclaimed rewards
     * @field staking_reward_claimed  - amount of stake claimed rewards
     * @field consensus_debt - amount of requested consensus debt
     * @field consensus_reward_unclaimed - amount of consensus unclaimed rewards
     * @field consensus_reward_claimed  - amount of consensus claimed rewards
     */
    struct [[eosio::table]] evm_staker_row {
        uint64_t id;
        checksum160 proxy;
        checksum160 staker;
        name validator;
        asset quantity;
        uint64_t stake_debt;
        asset staking_reward_unclaimed;
        asset staking_reward_claimed;
        uint64_t consensus_debt;
        asset consensus_reward_unclaimed;
        asset consensus_reward_claimed;
        uint64_t primary_key() const { return id; }
        uint64_t by_validator() const { return validator.value; }
        checksum256 by_staker() const { return xsat::utils::compute_id(staker); }
        checksum256 by_proxy() const { return xsat::utils::compute_id(proxy); }
        checksum256 by_staking_id() const { return compute_staking_id(proxy, staker, validator); }
    };
    typedef eosio::multi_index<
        "evmstakers"_n, evm_staker_row,
        eosio::indexed_by<"byvalidator"_n, const_mem_fun<evm_staker_row, uint64_t, &evm_staker_row::by_validator>>,
        eosio::indexed_by<"bystaker"_n, const_mem_fun<evm_staker_row, checksum256, &evm_staker_row::by_staker>>,
        eosio::indexed_by<"bystakingid"_n, const_mem_fun<evm_staker_row, checksum256, &evm_staker_row::by_staking_id>>,
        eosio::indexed_by<"byproxy"_n, const_mem_fun<evm_staker_row, checksum256, &evm_staker_row::by_proxy>>>
        evm_staker_table;

    /**
     * @brief native stake table.
     * @scope `get_self()`
     *
     * @field id - primary key
     * @field staker - staker address
     * @field validator - validator address
     * @field quantity - total number of staking
     * @field stake_debt - amount of requested stake debt
     * @field staking_reward_unclaimed - amount of stake unclaimed rewards
     * @field staking_reward_claimed  - amount of stake claimed rewards
     * @field consensus_debt - amount of requested consensus debt
     * @field consensus_reward_unclaimed - amount of consensus unclaimed rewards
     * @field consensus_reward_claimed  - amount of consensus claimed rewards
     *
     */
    struct [[eosio::table]] native_staker_row {
        uint64_t id;
        name staker;
        name validator;
        asset quantity;
        uint64_t stake_debt;
        asset staking_reward_unclaimed;
        asset staking_reward_claimed;
        uint64_t consensus_debt;
        asset consensus_reward_unclaimed;
        asset consensus_reward_claimed;
        uint64_t primary_key() const { return id; }
        uint64_t by_validator() const { return validator.value; }
        uint64_t by_staker() const { return staker.value; }
        uint128_t by_staking_id() const { return compute_staking_id(staker, validator); }
    };
    typedef eosio::multi_index<
        "stakers"_n, native_staker_row,
        eosio::indexed_by<"byvalidator"_n,
                          const_mem_fun<native_staker_row, uint64_t, &native_staker_row::by_validator>>,
        eosio::indexed_by<"bystaker"_n, const_mem_fun<native_staker_row, uint64_t, &native_staker_row::by_staker>>,
        eosio::indexed_by<"bystakingid"_n,
                          const_mem_fun<native_staker_row, uint128_t, &native_staker_row::by_staking_id>>>
        native_staker_table;

    /**
     * @brief validator table.
     * @scope `get_self()`
     *
     * @field owner - primary key
     * @field reward_recipient - get rewards to address
     * @field memo - memo when receiving rewards
     * @field commission_rate - commission rate
     * @field quantity - total number of staking
     * @field stake_acc_per_share - staking rewards earnings per share
     * @field consensus_acc_per_share - consensus reward earnings per share
     * @field stake_debt - amount of requested stake debt
     * @field staking_reward_unclaimed - amount of stake unclaimed rewards
     * @field staking_reward_claimed  - amount of stake claimed rewards
     * @field consensus_debt - amount of requested consensus debt
     * @field consensus_reward_unclaimed - amount of consensus unclaimed rewards
     * @field consensus_reward_claimed  - amount of consensus claimed rewards
     * @field latest_staking_time - latest staking or unstaking time
     * @field latest_reward_block - latest reward block
     * @field latest_reward_time - latest reward time
     * @field disabled_staking - whether to disable staking
     *
     */
    struct [[eosio::table]] validator_row {
        name owner;
        name reward_recipient;
        string memo;
        uint64_t commission_rate;
        asset quantity;
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
        uint64_t by_total_staking() const { return quantity.amount; }
        uint64_t by_disabled_staking() const { return disabled_staking; }
    };
    typedef eosio::multi_index<
        "validators"_n, validator_row,
        eosio::indexed_by<"bystate"_n, const_mem_fun<validator_row, uint64_t, &validator_row::by_disabled_staking>>,
        eosio::indexed_by<"bystaked"_n, const_mem_fun<validator_row, uint64_t, &validator_row::by_total_staking>>>
        validator_table;

    /**
     * @brief stat table.
     * @scope `get_self()`
     *
     * @field owner - primary key
     * @field reward_recipient - get rewards to address
     *
     */
    struct [[eosio::table]] stat_row {
        asset total_staking = {0, BTC_SYMBOL};
    };
    typedef eosio::singleton<"stat"_n, stat_row> stat_table;

    /**
     * Add whitelist action.
     * @scope `get_self()`
     *
     * @param type - Type of whitelist @see WHITELIST_TYPES
     * @param account - the account to be whitelisted.
     *
     */
    [[eosio::action]]
    void addwhitelist(const name& type, const name& account);

    /**
     * Delete whitelist action.
     * @scope `get_self()`
     *
     * @param type - Type of whitelist @see WHITELIST_TYPES
     * @param account - to delete the whitelisted account.
     *
     */
    [[eosio::action]]
    void delwhitelist(const name& type, const name& account);

    /**
     * Add evm proxy action.
     * @scope `get_self()`
     *
     * @param proxy - evm proxy address
     *
     */
    [[eosio::action]]
    void addevmproxy(const checksum160& proxy);

    /**
     * Delete evm proxy action.
     * @scope `get_self()`
     *
     * @param proxy - evm proxy address
     *
     */
    [[eosio::action]]
    void delevmproxy(const checksum160& proxy);

    /**
     * Registration validator action.
     * @auth `validator`
     *
     * @param validator - validator account
     * @param financial_account - the account that receives the income
     * @param commission_rate - commission ratio, decimal is 10^8
     *
     */
    [[eosio::action]]
    void regvalidator(const name& validator, const string& financial_account, const uint64_t commission_rate);

    /**
     * proxy registration validator action.
     * @auth `proxy`
     *
     * @param proxy - the agent registering the account must be in the whitelist
     * @param validator - validator account
     * @param financial_account - the account that receives the income
     * @param commission_rate - commission ratio, decimal is 10^8
     *
     */
    [[eosio::action]]
    void proxyreg(const name& proxy, const name& validator, const string& financial_account,
                  const uint64_t commission_rate);

    /**
     * validator config action.
     * @auth `validator`
     *
     * @param validator - validator account
     * @param commission_rate - commission ratio, decimal is 10^8
     * @param financial_account - the account that receives the income
     *
     */
    [[eosio::action]]
    void config(const name& validator, const uint64_t commission_rate, const string& financial_account);

    /**
     * Disable staking action.
     * @auth `get_self()`
     *
     * @param validator - validator account
     * @param disabled_staking - the status of the pledge token, true means disabled, false means enabled
     *
     */
    [[eosio::action]]
    void setstatus(const name& validator, const bool disabled_staking);

    /**
     * Native stake action.
     * @auth `staking.xsat`
     *
     * @param staker - pledge account
     * @param validator - validator account
     * @param quantity - pledge quantity
     *
     */
    [[eosio::action]]
    void stake(const name& staker, const name& validator, const asset& quantity);

    /**
     * Native unstake action.
     * @auth `staking.xsat`
     *
     * @param staker - staker account
     * @param validator - validator account
     * @param quantity - cancel pledge quantity
     *
     */
    [[eosio::action]]
    void unstake(const name& staker, const name& validator, const asset& quantity);

    /**
     * Native change staking action.
     * @auth `staking.xsat`
     *
     * @param staker - staker account
     * @param old_validator - old validator address
     * @param new_validator - new validator address
     * @param quantity - change the amount of pledge
     *
     */
    [[eosio::action]]
    void newstake(const name& staker, const name& old_validator, const name& new_validator, const asset& quantity);

    /**
     * Native claim reward action.
     * @auth `staker`
     *
     * @param staker - staker account
     * @param validator - validator account
     *
     */
    [[eosio::action]]
    void claim(const name& staker, const name& validator);

    /**
     * Evm stake action.
     * @auth scope is `evmcaller` whitelist account
     *
     * @param caller - the account that calls the method
     * @param proxy - proxy address
     * @param staker - staker address
     * @param validator - validator address
     * @param quantity - total number of stake
     *
     */
    [[eosio::action]]
    void evmstake(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator,
                  const asset& quantity);

    /**
     * Evm unstake action.
     * @auth scope is `evmcaller` whitelist account
     *
     * @param caller - the account that calls the method
     * @param proxy - proxy address
     * @param staker - staker address
     * @param validator - validator address
     * @param quantity - cancel pledge quantity
     *
     */
    [[eosio::action]]
    void evmunstake(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator,
                    const asset& quantity);

    /**
     * Evm change stake action.
     * @auth scope is `evmcaller` whitelist account
     *
     * @param caller - the account that calls the method
     * @param proxy - proxy address
     * @param staker - staker address
     * @param old_validator - old validator address
     * @param new_validator - new validator address
     * @param quantity - change the amount of pledge
     *
     */
    [[eosio::action]]
    void evmnewstake(const name& caller, const checksum160& proxy, const checksum160& staker, const name& old_validator,
                     const name& new_validator, const asset& quantity);

    /**
     * Evm claim reward action.
     * @auth scope is `evmcaller` whitelist account
     *
     * @param caller - the account that calls the method
     * @param proxy - proxy address
     * @param staker - staker address
     * @param validator - validator address
     *
     */
    [[eosio::action]]
    void evmclaim(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator);

    /**
     * validator claim reward action.
     * @auth validator->reward_recipient or evmutil.xsat
     *
     * @param validator - validator account
     *
     */
    [[eosio::action]]
    void vdrclaim(const name& validator);

    struct reward_details_row {
        name validator;
        asset staking_rewards;
        asset consensus_rewards;
    };

    /**
     * Distribute  rewards action.
     * @auth `rwddist.xsat`
     *
     * @param height - distributed block height
     * @param rewards - reward details
     *
     */
    [[eosio::action]]
    void distribute(const uint64_t height, const vector<reward_details_row> rewards);

    [[eosio::on_notify("*::transfer")]]
    void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    // logs
    [[eosio::action]]
    void validatorlog(const name& proxy, const name& validator, const string& financial_account,
                      const uint64_t commission_rate) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void stakelog(const name& staker, const name& validator, const asset& quantity, const asset& validator_staking) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void unstakelog(const name& staker, const name& validator, const asset& quantity, const asset& validator_staking) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void newstakelog(const name& staker, const name& old_validator, const name& new_validator, const asset& quantity,
                     const asset& old_validator_staking, const asset& new_validator_staking) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void claimlog(const name& staker, const name& validator, const asset& quantity) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void evmstakelog(const checksum160& proxy, const checksum160& staker, const name& validator, const asset& quantity,
                     const asset& validator_staking) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void evmunstlog(const checksum160& proxy, const checksum160& staker, const name& validator, const asset& quantity,
                    const asset& validator_staking) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void evmnewstlog(const checksum160& proxy, const checksum160& staker, const name& old_validator,
                     const name& new_validator, const asset& quantity, const asset& old_validator_staking,
                     const asset& new_validator_staking) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void evmclaimlog(const checksum160& proxy, const checksum160& staker, const name& validator,
                     const asset& quantity) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void vdrclaimlog(const name& validator, const string& reward_recipient, const asset& quantity) {
        require_auth(get_self());
    }

    using stake_action = eosio::action_wrapper<"stake"_n, &endorse_manage::stake>;
    using unstake_action = eosio::action_wrapper<"unstake"_n, &endorse_manage::unstake>;
    using evm_staker_action = eosio::action_wrapper<"evmstake"_n, &endorse_manage::evmstake>;
    using evm_unstake_action = eosio::action_wrapper<"evmunstake"_n, &endorse_manage::evmunstake>;
    using distribute_action = eosio::action_wrapper<"distribute"_n, &endorse_manage::distribute>;
    using validatorlog_action = eosio::action_wrapper<"validatorlog"_n, &endorse_manage::validatorlog>;
    using stakelog_action = eosio::action_wrapper<"stakelog"_n, &endorse_manage::stakelog>;
    using unstakelog_action = eosio::action_wrapper<"unstakelog"_n, &endorse_manage::unstakelog>;
    using newstakelog_action = eosio::action_wrapper<"newstakelog"_n, &endorse_manage::newstakelog>;
    using claimlog_action = eosio::action_wrapper<"claimlog"_n, &endorse_manage::claimlog>;
    using evmstakelog_action = eosio::action_wrapper<"evmstakelog"_n, &endorse_manage::evmstakelog>;
    using evmunstlog_action = eosio::action_wrapper<"evmunstlog"_n, &endorse_manage::evmunstlog>;
    using evmnewstlog_action = eosio::action_wrapper<"evmnewstlog"_n, &endorse_manage::evmnewstlog>;
    using evmclaimlog_action = eosio::action_wrapper<"evmclaimlog"_n, &endorse_manage::evmclaimlog>;
    using vdrclaimlog_action = eosio::action_wrapper<"vdrclaimlog"_n, &endorse_manage::vdrclaimlog>;

    static checksum256 compute_staking_id(const checksum160& proxy, const checksum160& staker, const name& validator) {
        vector<char> result;
        result.resize(48);
        eosio::datastream<char*> ds(result.data(), result.size());
        ds << proxy;
        ds << staker;
        ds << validator;
        return eosio::sha256((char*)result.data(), result.size());
    }

    static uint128_t compute_staking_id(const name& staker, const name& validator) {
        return uint128_t(staker.value) << 64 | validator.value;
    }

   private:
    global_id_table _global_id = global_id_table(_self, _self.value);
    validator_table _validator = validator_table(_self, _self.value);
    evm_staker_table _evm_stake = evm_staker_table(_self, _self.value);
    native_staker_table _native_stake = native_staker_table(_self, _self.value);
    evm_proxy_table _evm_proxy = evm_proxy_table(_self, _self.value);
    stat_table _stat = stat_table(_self, _self.value);

    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);
    uint64_t next_staking_id();

    asset evm_stake_without_auth(const checksum160& proxy, const checksum160& staker, const name& validator,
                                 const asset& quantity);
    asset evm_unstake_without_auth(const checksum160& proxy, const checksum160& staker, const name& validator,
                                   const asset& quantity);
    asset stake_without_auth(const name& staker, const name& validator, const asset& quantity);
    asset unstake_without_auth(const name& staker, const name& validator, const asset& quantity);

    template <typename T, typename C>
    void staking_change(validator_table::const_iterator& validator_itr, T& _stake, C& stake_itr, const asset& quantity);

    template <typename T, typename C>
    void update_staking_reward(const uint128_t stake_acc_per_share, const uint128_t consensus_acc_per_share,
                               const uint64_t pre_stake, const uint64_t now_stake, T& _stake, C& stake_itr);

    void update_validator_reward(const uint64_t height, const name& validator, const uint64_t staking_reward,
                                 const uint64_t consensus_reward);

    void register_validator(const name& proxy, const name& validator, const string& financial_account,
                            const uint64_t commission_rate);
};
