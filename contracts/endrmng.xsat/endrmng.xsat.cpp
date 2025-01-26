#include <btc.xsat/btc.xsat.hpp>
#include <endrmng.xsat/endrmng.xsat.hpp>
#include "../internal/safemath.hpp"

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

//@auth get_self()
[[eosio::action]]
void endorse_manage::setdonateacc(const string& donation_account, const uint16_t min_donate_rate) {
    require_auth(get_self());

    bool is_eos_address = donation_account.size() <= 12;
    if (is_eos_address) {
        check(is_account(name(donation_account)), "endrmng.xsat::setdonateacc: donation account does not exists");
    } else {
        check(xsat::utils::is_valid_evm_address(donation_account), "endrmng.xsat::setdonateacc: invalid donation account");
    }
    check(min_donate_rate <= RATE_BASE_10000,
          "endrmng.xsat::setdonateacc: min_donate_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    auto config = _config.get_or_default();
    config.donation_account = donation_account;
    config.min_donate_rate = min_donate_rate;
    _config.set(config, get_self());
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::addwhitelist(const name& type, const name& account) {
    require_auth(get_self());

    check(WHITELIST_TYPES.find(type) != WHITELIST_TYPES.end(), "endrmng.xsat::addwhitelist: type invalid");
    check(is_account(account), "endrmng.xsat::addwhitelist: account does not exist");

    whitelist_table _whitelist(get_self(), type.value);
    auto whitelist_itr = _whitelist.find(account.value);
    check(whitelist_itr == _whitelist.end(), "endrmng.xsat::addwhitelist: whitelist account already exists");

    _whitelist.emplace(get_self(), [&](auto& row) { row.account = account; });
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::delwhitelist(const name& type, const name& account) {
    require_auth(get_self());

    // check
    check(WHITELIST_TYPES.find(type) != WHITELIST_TYPES.end(), "endrmng.xsat::delwhitelist: [type] is invalid");

    // erase whitelist
    whitelist_table _whitelist(get_self(), type.value);
    auto whitelist_itr = _whitelist.require_find(account.value, "endrmng.xsat::delwhitelist: whitelist account does not exist");
    _whitelist.erase(whitelist_itr);
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::addevmproxy(const name& caller, const checksum160& proxy) {
    require_auth(get_self());

    whitelist_table _whitelist(get_self(), "evmcaller"_n.value);
    auto whitelist_itr
        = _whitelist.require_find(caller.value, "endrmng.xsat::addevmproxy: caller is not in the `evmcaller` whitelist");

    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), caller.value);
    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    auto evm_proxy_itr = evm_proxy_idx.find(xsat::utils::compute_id(proxy));
    check(evm_proxy_itr == evm_proxy_idx.end(), "endrmng.xsat::addevmproxy: proxy already exists");
    _evm_proxy.emplace(get_self(), [&](auto& row) {
        row.id = _evm_proxy.available_primary_key();
        row.proxy = proxy;
    });
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::delevmproxy(const name& caller, const checksum160& proxy) {
    require_auth(get_self());

    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), caller.value);
    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    auto evm_proxy_itr
        = evm_proxy_idx.require_find(xsat::utils::compute_id(proxy), "endrmng.xsat::delevmproxy: [evmproxies] does not exist");

    evm_proxy_idx.erase(evm_proxy_itr);
}

//@auth validator
[[eosio::action]]
void endorse_manage::regvalidator(const name& validator, const string& financial_account, const uint16_t commission_rate) {
    require_auth(validator);
    register_validator({}, validator, financial_account, commission_rate);
}

//@auth proxy
[[eosio::action]]
void endorse_manage::proxyreg(const name& proxy, const name& validator, const string& financial_account,
                              const uint16_t commission_rate) {
    require_auth(proxy);
    whitelist_table _whitelist(get_self(), "proxyreg"_n.value);
    _whitelist.require_find(proxy.value, "endrmng.xsat::proxyreg: caller is not in the `proxyreg` whitelist");

    check(is_account(validator), "endrmng.xsat::proxyreg: validator account does not exists");

    register_validator(proxy, validator, financial_account, commission_rate);
}

void endorse_manage::register_validator(const name& proxy, const name& validator, const string& financial_account,
                                        const uint16_t commission_rate) {

    // check consensus version
    auto config = _consensus_config.get_or_default();
    check(config.version == 1, "endrmng.xsat::regvalidator: consensus version must be 1");

    check(commission_rate <= RATE_BASE_10000,
          "endrmng.xsat::regvalidator: commission_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));
    bool is_eos_address = financial_account.size() <= 12;
    if (is_eos_address) {
        check(is_account(name(financial_account)), "endrmng.xsat::regvalidator: financial account does not exists");
    } else {
        check(xsat::utils::is_valid_evm_address(financial_account), "endrmng.xsat::regvalidator: invalid financial account");
    }
    auto validator_itr = _validator.find(validator.value);
    check(validator_itr == _validator.end(), "endrmng.xsat::regvalidator: [validators] already exists");
    _validator.emplace(get_self(), [&](auto& row) {
        row.owner = validator;
        row.commission_rate = commission_rate;
        if (is_eos_address) {
            row.reward_recipient = name(financial_account);
            row.memo = "";
        } else {
            row.reward_recipient = ERC20_CONTRACT;
            row.memo = financial_account;
        }
        row.quantity = asset{0, BTC_SYMBOL};
        row.xsat_quantity = asset{0, XSAT_SYMBOL};
        row.qualification = {0, BTC_SYMBOL};
        row.donate_rate = 0;
        row.total_donated = {0, XSAT_SYMBOL};
        row.staking_reward_unclaimed = asset{0, XSAT_SYMBOL};
        row.staking_reward_claimed = asset{0, XSAT_SYMBOL};
        row.consensus_reward_unclaimed = asset{0, XSAT_SYMBOL};
        row.consensus_reward_claimed = asset{0, XSAT_SYMBOL};
        row.total_consensus_reward = asset{0, XSAT_SYMBOL};
        row.consensus_reward_balance = asset{0, XSAT_SYMBOL};
        row.total_staking_reward = asset{0, XSAT_SYMBOL};
        row.staking_reward_balance = asset{0, XSAT_SYMBOL};
    });
    // log
    endorse_manage::validatorlog_action _validatorlog(get_self(), {get_self(), "active"_n});
    _validatorlog.send(proxy, validator, financial_account, commission_rate);
}

//@auth validator
[[eosio::action]]
void endorse_manage::config(const name& validator, const optional<uint16_t>& commission_rate,
                            const optional<string>& financial_account) {
    require_auth(validator);
    check(commission_rate.has_value() || financial_account.has_value(),
          "endrmng.xsat::config: commissionrate and financial account cannot be empty at the same time");
    check(!commission_rate.has_value() || *commission_rate <= RATE_BASE_10000,
          "endrmng.xsat::config: commission_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::config: [validators] does not exists");
    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        if (commission_rate.has_value()) {
            row.commission_rate = *commission_rate;
        }
        if (financial_account.has_value()) {
            bool is_eos_account = financial_account->size() <= 12;
            if (is_eos_account) {
                check(is_account(name(*financial_account)), "endrmng.xsat::config: financial account does not exists");
                row.reward_recipient = name(*financial_account);
                row.memo = "";
            } else {
                check(xsat::utils::is_valid_evm_address(*financial_account), "endrmng.xsat::config: invalid financial account");
                row.reward_recipient = ERC20_CONTRACT;
                row.memo = *financial_account;
            }
        }
    });

    // log
    endorse_manage::configlog_action _configlog(get_self(), {get_self(), "active"_n});
    if (validator_itr->reward_recipient == ERC20_CONTRACT) {
        _configlog.send(validator, validator_itr->commission_rate, validator_itr->memo);
    } else {
        _configlog.send(validator, validator_itr->commission_rate, validator_itr->reward_recipient.to_string());
    }
}

//@auth validator
[[eosio::action]]
void endorse_manage::setdonate(const name& validator, const uint16_t donate_rate) {
    require_auth(validator);

    check(donate_rate <= RATE_BASE_10000,
          "endrmng.xsat::setdonate: donate_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::setdonate: [validator] does not exists");

    _validator.modify(validator_itr, same_payer, [&](auto& row) { row.donate_rate = donate_rate; });

    // log
    endorse_manage::setdonatelog_action _setdonatelog(get_self(), {get_self(), "active"_n});
    _setdonatelog.send(validator, donate_rate);
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::setstatus(const name& validator, const bool disabled_staking) {
    require_auth(get_self());

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::setstatus: [validators] does not exists");
    _validator.modify(validator_itr, same_payer, [&](auto& row) { row.disabled_staking = disabled_staking; });
}

//==============================================================  staking btc
//=========================================================

//@auth staking.xsat
[[eosio::action]]
void endorse_manage::stake(const name& staker, const name& validator, const asset& quantity) {
    require_auth(STAKING_CONTRACT);

    asset validator_staking, validator_qualification;
    std::tie(validator_staking, validator_qualification) = stake_without_auth(staker, validator, quantity, quantity);

    // log
    endorse_manage::stakelog_action _stakelog(get_self(), {get_self(), "active"_n});
    _stakelog.send(staker, validator, quantity, validator_staking, validator_qualification);
}

//@auth staking.xsat
[[eosio::action]]
void endorse_manage::unstake(const name& staker, const name& validator, const asset& quantity) {
    require_auth(STAKING_CONTRACT);

    asset validator_staking, validator_qualification;
    std::tie(validator_staking, validator_qualification) = unstake_without_auth(staker, validator, quantity, quantity);

    // log
    endorse_manage::unstakelog_action _unstakelog(get_self(), {get_self(), "active"_n});
    _unstakelog.send(staker, validator, quantity, validator_staking, validator_qualification);
}

//@auth staker
[[eosio::action]]
void endorse_manage::newstake(const name& staker, const name& old_validator, const name& new_validator, const asset& quantity) {
    require_auth(staker);

    asset old_validator_staking, old_validator_qualification;
    std::tie(old_validator_staking, old_validator_qualification)
        = unstake_without_auth(staker, old_validator, quantity, quantity);
    asset new_validator_staking, new_validator_qualification;
    std::tie(new_validator_staking, new_validator_qualification) = stake_without_auth(staker, new_validator, quantity, quantity);

    // log
    endorse_manage::newstakelog_action _newstakelog(get_self(), {get_self(), "active"_n});
    _newstakelog.send(staker, old_validator, new_validator, quantity, old_validator_staking, old_validator_qualification,
                      new_validator_staking, new_validator_qualification);
}

//@auth staker
[[eosio::action]]
void endorse_manage::claim(const name& staker, const name& validator, const uint16_t donate_rate) {
    require_auth(staker);

    check(donate_rate <= RATE_BASE_10000,
          "endrmng.xsat::claim: donate_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    auto native_staker_idx = _native_stake.get_index<"bystakingid"_n>();
    auto native_staker_itr
        = native_staker_idx.require_find(compute_staking_id(staker, validator), "endrmng.xsat::claim: [stakers] does not exists");

    auto validator_itr
        = _validator.require_find(native_staker_itr->validator.value, "endrmng.xsat::claim: [validators] does not exists");
    // update reward
    update_staking_reward(validator_itr->stake_acc_per_share, validator_itr->consensus_acc_per_share,
                          native_staker_itr->quantity.amount, native_staker_itr->quantity.amount, native_staker_idx,
                          native_staker_itr);

    auto staking_reward_unclaimed = native_staker_itr->staking_reward_unclaimed;
    auto consensus_reward_unclaimed = native_staker_itr->consensus_reward_unclaimed;
    auto claimable = staking_reward_unclaimed + consensus_reward_unclaimed;
    check(claimable.amount > 0, "endrmng.xsat::claim: no balance to claim");

    asset donated_amount = claimable * donate_rate / RATE_BASE_10000;
    asset to_staker = claimable - donated_amount;

    native_staker_idx.modify(native_staker_itr, same_payer, [&](auto& row) {
        row.total_donated += donated_amount;
        row.staking_reward_claimed += staking_reward_unclaimed;
        row.consensus_reward_claimed += consensus_reward_unclaimed;
        row.staking_reward_unclaimed -= staking_reward_unclaimed;
        row.consensus_reward_unclaimed -= consensus_reward_unclaimed;
    });

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.staking_reward_balance -= staking_reward_unclaimed;
        row.consensus_reward_balance -= consensus_reward_unclaimed;
    });

    // transfer donate
    if (donated_amount.amount > 0) {
        auto config = _config.get();
        token_transfer(get_self(), config.donation_account, extended_asset{donated_amount, EXSAT_CONTRACT});

        auto stat = _stat.get_or_default();
        stat.xsat_total_donated += donated_amount;
        _stat.set(stat, get_self());
    }

    if (to_staker.amount > 0) {
        token_transfer(get_self(), native_staker_itr->staker, {to_staker, EXSAT_CONTRACT}, "claim reward");
    }

    endorse_manage::claimlog_action _claimlog(get_self(), {get_self(), "active"_n});
    _claimlog.send(staker, validator, claimable, donated_amount, native_staker_itr->total_donated);
}

// @auth scope is `evmcaller` evmproxies account
[[eosio::action]]
void endorse_manage::evmstake(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator,
                              const asset& quantity) {
    require_auth(caller);

    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), caller.value);
    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    evm_proxy_idx.require_find(xsat::utils::compute_id(proxy), "endrmng.xsat::evmstake: [evmproxies] does not exist");

    asset validator_staking, validator_qualification;
    std::tie(validator_staking, validator_qualification) = evm_stake_without_auth(proxy, staker, validator, quantity, quantity);

    // log
    endorse_manage::evmstakelog_action _evmstakelog(get_self(), {get_self(), "active"_n});
    _evmstakelog.send(proxy, staker, validator, quantity, validator_staking, validator_qualification);
}

// @auth scope is `evmcaller` evmproxies account
[[eosio::action]]
void endorse_manage::evmunstake(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator,
                                const asset& quantity) {
    require_auth(caller);

    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), caller.value);
    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    evm_proxy_idx.require_find(xsat::utils::compute_id(proxy), "endrmng.xsat::evmunstake: [evmproxies] does not exist");

    asset validator_staking, validator_qualification;

    std::tie(validator_staking, validator_qualification) = evm_unstake_without_auth(proxy, staker, validator, quantity, quantity);

    // log
    endorse_manage::evmunstlog_action _evmunstlog(get_self(), {get_self(), "active"_n});
    _evmunstlog.send(proxy, staker, validator, quantity, validator_staking, validator_qualification);
}

// @auth scope is `evmcaller` evmproxies account
[[eosio::action]]
void endorse_manage::evmnewstake(const name& caller, const checksum160& proxy, const checksum160& staker,
                                 const name& old_validator, const name& new_validator, const asset& quantity) {
    require_auth(caller);

    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), caller.value);
    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    evm_proxy_idx.require_find(xsat::utils::compute_id(proxy), "endrmng.xsat::evmnewstake: [evmproxies] does not exist");

    // unstake
    asset old_validator_staking, old_validator_qualification;
    std::tie(old_validator_staking, old_validator_qualification)
        = evm_unstake_without_auth(proxy, staker, old_validator, quantity, quantity);

    asset new_validator_staking, new_validator_qualification;
    std::tie(new_validator_staking, new_validator_qualification)
        = evm_stake_without_auth(proxy, staker, new_validator, quantity, quantity);

    // log
    endorse_manage::evmnewstlog_action _evmnewstlog(get_self(), {get_self(), "active"_n});
    _evmnewstlog.send(proxy, staker, old_validator, new_validator, quantity, old_validator_staking, old_validator_qualification,
                      new_validator_staking, new_validator_qualification);
}

// @auth scope is `evmcaller` whitelist account
[[eosio::action]]
void endorse_manage::evmclaim(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator) {
    evm_claim(caller, proxy, staker, validator, 0);
}

// @auth scope is `evmcaller` whitelist account
[[eosio::action]]
void endorse_manage::evmclaim2(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator,
                               const uint16_t donate_rate) {
    evm_claim(caller, proxy, staker, validator, donate_rate);
}

void endorse_manage::evm_claim(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator,
                               const uint16_t donate_rate) {
    require_auth(caller);

    check(donate_rate <= RATE_BASE_10000,
          "endrmng.xsat::evmclaim: donate_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    whitelist_table _whitelist(get_self(), "evmcaller"_n.value);
    _whitelist.require_find(caller.value, "endrmng.xsat::evmclaim: caller is not in the `evmcaller` whitelist");

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto evm_staker_itr = evm_staker_idx.require_find(
        compute_staking_id(proxy, staker, validator), "endrmng.xsat::evmclaim: [evmstakers] does not exists");

    auto validator_itr
        = _validator.require_find(evm_staker_itr->validator.value, "endrmng.xsat::evmclaim: [validators] does not exists");
    update_staking_reward(validator_itr->stake_acc_per_share, validator_itr->consensus_acc_per_share,
                          evm_staker_itr->quantity.amount, evm_staker_itr->quantity.amount, evm_staker_idx, evm_staker_itr);

    auto staking_reward_unclaimed = evm_staker_itr->staking_reward_unclaimed;
    auto consensus_reward_unclaimed = evm_staker_itr->consensus_reward_unclaimed;
    auto claimable = staking_reward_unclaimed + consensus_reward_unclaimed;
    check(claimable.amount > 0, "endrmng.xsat::evmclaim: no balance to claim");

    auto config = _config.get();

    asset validator_donated_amount = {0, XSAT_SYMBOL};
    asset staker_donated_amount = {0, XSAT_SYMBOL};
    auto credit_proxy_idx = _credit_proxy.get_index<"byproxy"_n>();
    auto credit_proxy_itr = credit_proxy_idx.find(xsat::utils::compute_id(proxy));
    bool is_credit_staking = credit_proxy_itr != credit_proxy_idx.end();

    // Use validator's donate_rate for credit staking, otherwise use input donate_rate
    if (is_credit_staking) {
        auto donate_rate = std::max(config.min_donate_rate.value_or(uint16_t(0)), validator_itr->donate_rate);
        validator_donated_amount = claimable * donate_rate / RATE_BASE_10000;
    } else {
        staker_donated_amount = claimable * donate_rate / RATE_BASE_10000;
    }

    evm_staker_idx.modify(evm_staker_itr, same_payer, [&](auto& row) {
        row.total_donated += staker_donated_amount;
        row.staking_reward_claimed += staking_reward_unclaimed;
        row.consensus_reward_claimed += consensus_reward_unclaimed;
        row.staking_reward_unclaimed -= staking_reward_unclaimed;
        row.consensus_reward_unclaimed -= consensus_reward_unclaimed;
    });

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.total_donated += validator_donated_amount;
        row.staking_reward_balance -= staking_reward_unclaimed;
        row.consensus_reward_balance -= consensus_reward_unclaimed;
    });

    auto donated_amount = staker_donated_amount + validator_donated_amount;
    // transfer donate
    if (donated_amount.amount > 0) {
        token_transfer(get_self(), config.donation_account, extended_asset{donated_amount, EXSAT_CONTRACT});

        auto stat = _stat.get_or_default();
        stat.xsat_total_donated += donated_amount;
        _stat.set(stat, get_self());
    }

    // transfer reward
    auto to_staker = claimable - donated_amount;
    if (to_staker.amount > 0) {
        token_transfer(
            get_self(), ERC20_CONTRACT, {to_staker, EXSAT_CONTRACT}, "0x" + xsat::utils::sha1_to_hex(evm_staker_itr->staker));
    }

    // log
    endorse_manage::evmclaimlog_action _evmclaimlog(get_self(), {get_self(), "active"_n});
    _evmclaimlog.send(proxy, staker, validator, claimable, staker_donated_amount, validator_donated_amount,
                      evm_staker_itr->total_donated, validator_itr->total_donated);
}

//@auth validator
[[eosio::action]]
void endorse_manage::vdrclaim(const name& validator) {
    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::vdrclaim: [validators] does not exists");

    if (validator_itr->reward_recipient == ERC20_CONTRACT) {
        require_auth(EVM_UTIL_CONTRACT);
    } else {
        require_auth(validator_itr->reward_recipient);
    }

    auto staking_reward_unclaimed = validator_itr->staking_reward_unclaimed;
    auto consensus_reward_unclaimed = validator_itr->consensus_reward_unclaimed;
    auto claimable = staking_reward_unclaimed + consensus_reward_unclaimed;
    check(claimable.amount > 0, "endrmng.xsat::vdrclaim: no balance to claim");

    auto config = _config.get();
    auto donate_rate = validator_itr->donate_rate;
    auto donated_amount = claimable * donate_rate / RATE_BASE_10000;
    auto to_validator = claimable - donated_amount;

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.total_donated += donated_amount;
        row.staking_reward_claimed += staking_reward_unclaimed;
        row.consensus_reward_claimed += consensus_reward_unclaimed;
        row.staking_reward_unclaimed -= staking_reward_unclaimed;
        row.consensus_reward_unclaimed -= consensus_reward_unclaimed;
        row.consensus_reward_balance -= consensus_reward_unclaimed;
        row.staking_reward_balance -= staking_reward_unclaimed;
    });

    // transfer donate
    if (donated_amount.amount > 0) {
        token_transfer(get_self(), config.donation_account, extended_asset{donated_amount, EXSAT_CONTRACT});

        auto stat = _stat.get_or_default();
        stat.xsat_total_donated += donated_amount;
        _stat.set(stat, get_self());
    }

    // transfer reward
    if (to_validator.amount > 0) {
        token_transfer(get_self(), validator_itr->reward_recipient, {to_validator, EXSAT_CONTRACT}, validator_itr->memo);
    }

    // log
    string reward_recipient
        = validator_itr->reward_recipient == ERC20_CONTRACT ? validator_itr->memo : validator_itr->reward_recipient.to_string();
    endorse_manage::vdrclaimlog_action _vdrclaimlog(get_self(), {get_self(), "active"_n});
    _vdrclaimlog.send(validator, reward_recipient, claimable, donated_amount, validator_itr->total_donated);
}

std::pair<asset, asset> endorse_manage::evm_stake_without_auth(const checksum160& proxy, const checksum160& staker,
                                                               const name& validator, const asset& quantity,
                                                               const asset& qualification) {
    check(quantity.amount > 0, "endrmng.xsat::evmstake: quantity must be greater than 0");
    check(quantity.symbol == BTC_SYMBOL, "endrmng.xsat::evmstake: quantity symbol must be BTC");
    check(quantity.amount <= BTC_SUPPLY, "endrmng.xsat::evmstake: quantity must be less than [btc.xsat/BTC] max_supply");

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::evmstake: [validators] does not exists");
    check(!validator_itr->disabled_staking, "endrmng.xsat::evmstake: the current validator's staking status is disabled");

    if (validator_itr->role.has_value()) {

        check(validator_itr->role.value() == 0, "endrmng.xsat::evmstake: only BTC validator can be staked");
    }

    // check base stake
    auto consens_config = _consensus_config.get_or_default();
    if (validator_itr->active_flag.value() == 0 && validator_itr->stake_address.has_value()) {

        // need to check base stake
        check(validator_itr->stake_address.value() == staker,
              "endrmng.xsat::evmstake: only the validator's stake address can stake");
    }

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto stake_itr = evm_staker_idx.find(compute_staking_id(proxy, staker, validator));

    // V2 check base stake amount
    auto active_flag = validator_itr->active_flag.value();
    if (validator_itr->stake_address.has_value() && validator_itr->stake_address.value() == stake_itr->staker) {

        auto config = _consensus_config.get_or_default();
        auto stake_quantity = quantity;
        if (stake_itr != evm_staker_idx.end()) {

            stake_quantity += stake_itr->quantity;
        }
        if (stake_quantity >= config.btc_base_stake) {

            active_flag = 1;
        } else {

            active_flag = 0;
        }
    }

    if (stake_itr == evm_staker_idx.end()) {
        auto staking_id = next_staking_id();
        auto stake_itr = _evm_stake.emplace(get_self(), [&](auto& row) {
            row.id = staking_id;
            row.proxy = proxy;
            row.staker = staker;
            row.validator = validator;
            row.quantity = {0, BTC_SYMBOL};
            row.xsat_quantity = {0, XSAT_SYMBOL};
            row.total_donated = {0, XSAT_SYMBOL};
            row.staking_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.staking_reward_claimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_claimed = asset{0, XSAT_SYMBOL};
        });

        staking_change(validator_itr, _evm_stake, stake_itr, quantity, qualification, active_flag);
    } else {

        staking_change(validator_itr, evm_staker_idx, stake_itr, quantity, qualification, active_flag);
    }
    return std::make_pair(validator_itr->quantity, validator_itr->qualification);
}

std::pair<asset, asset> endorse_manage::evm_unstake_without_auth(const checksum160& proxy, const checksum160& staker,
                                                                 const name& validator, const asset& quantity,
                                                                 const asset& qualification) {
    check(quantity.amount > 0, "endrmng.xsat::evmunstake: quantity must be greater than 0");
    check(quantity.symbol == BTC_SYMBOL, "endrmng.xsat::evmunstake: quantity symbol must be BTC");

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto evm_staker_itr = evm_staker_idx.require_find(
        compute_staking_id(proxy, staker, validator), "endrmng.xsat::evmunstake: [evmstakers] does not exists");
    check(evm_staker_itr->quantity >= quantity, "endrmng.xsat::evmunstake: insufficient stake");

    auto validator_itr
        = _validator.require_find(evm_staker_itr->validator.value, "endrmng.xsat::evmunstake: [validators] does not exists");

    // V2 check base stake amount
    auto active_flag = validator_itr->active_flag.value();
    if (validator_itr->stake_address.has_value() && validator_itr->stake_address.value() == evm_staker_itr->staker) {

        auto config = _consensus_config.get_or_default();
        if (evm_staker_itr->quantity + quantity >= config.btc_base_stake) {

            active_flag = 1;
        } else {

            active_flag = 0;
        }
    }

    staking_change(validator_itr, evm_staker_idx, evm_staker_itr, -quantity, -qualification, active_flag);
    return std::make_pair(validator_itr->quantity, validator_itr->qualification);
}

std::pair<asset, asset> endorse_manage::stake_without_auth(const name& staker, const name& validator, const asset& quantity,
                                                           const asset& qualification) {
    check(quantity.amount > 0, "endrmng.xsat::stake: quantity must be greater than 0");
    check(quantity.symbol == BTC_SYMBOL, "endrmng.xsat::stake: quantity symbol must be BTC");

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::stake: [validators] does not exists");
    check(!validator_itr->disabled_staking, "endrmng.xsat::stake: the current validator's staking status is disabled");

    auto native_staker_idx = _native_stake.get_index<"bystakingid"_n>();
    auto stake_itr = native_staker_idx.find(compute_staking_id(staker, validator));

    auto staking_id = next_staking_id();
    if (stake_itr == native_staker_idx.end()) {
        auto stake_itr = _native_stake.emplace(get_self(), [&](auto& row) {
            row.id = staking_id;
            row.staker = staker;
            row.validator = validator;
            row.quantity = asset{0, BTC_SYMBOL};
            row.xsat_quantity = asset{0, XSAT_SYMBOL};
            row.total_donated = {0, XSAT_SYMBOL};
            row.staking_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.staking_reward_claimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_claimed = asset{0, XSAT_SYMBOL};
        });
        staking_change(validator_itr, _native_stake, stake_itr, quantity, qualification, std::nullopt);
    } else {

        staking_change(validator_itr, native_staker_idx, stake_itr, quantity, qualification, std::nullopt);
    }
    return std::make_pair(validator_itr->quantity, validator_itr->qualification);
}

std::pair<asset, asset> endorse_manage::unstake_without_auth(const name& staker, const name& validator, const asset& quantity,
                                                             const asset& qualification) {
    check(quantity.amount > 0, "endrmng.xsat::unstake: quantity must be greater than 0");
    check(quantity.symbol == BTC_SYMBOL, "endrmng.xsat::unstake: quantity symbol must be BTC");

    auto native_staker_idx = _native_stake.get_index<"bystakingid"_n>();
    auto native_staker_itr = native_staker_idx.require_find(
        compute_staking_id(staker, validator), "endrmng.xsat::unstake: [stakers] does not exists");
    check(native_staker_itr->quantity >= quantity, "endrmng.xsat::unstake: the number of unstakes exceeds the staking amount");

    auto validator_itr
        = _validator.require_find(native_staker_itr->validator.value, "endrmng.xsat::unstake: [validators] does not exists");

    staking_change(validator_itr, native_staker_idx, native_staker_itr, -quantity, -qualification, std::nullopt);
    return std::make_pair(validator_itr->quantity, validator_itr->qualification);
}

template <typename T, typename C>
void endorse_manage::staking_change(validator_table::const_iterator& validator_itr, T& _stake, C& stake_itr,
                                    const asset& quantity, const asset& qualification, const optional<uint8_t>& active_flag) {
    // update reward
    auto now_amount_for_validator = validator_itr->quantity + quantity;
    auto pre_amount_for_staker = stake_itr->quantity;
    auto now_amount_for_staker = stake_itr->quantity + quantity;

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.quantity = now_amount_for_validator;
        row.qualification += qualification;
        row.latest_staking_time = current_time_point();

        //
        if (active_flag.has_value()) {

            row.active_flag = active_flag.value();
        }
    });

    stat_row stat = _stat.get_or_default();
    stat.total_staking += quantity;
    _stat.set(stat, get_self());

    update_staking_reward(validator_itr->stake_acc_per_share, validator_itr->consensus_acc_per_share,
                          pre_amount_for_staker.amount, now_amount_for_staker.amount, _stake, stake_itr);
}

void endorse_manage::update_validator_reward(const uint64_t height, const name& validator, const uint64_t staking_rewards,
                                             const uint64_t consensus_rewards) {
    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat: [validators] does not exists");
    check(validator_itr->latest_reward_block < height, "endrmng.xsat: the block height has been rewarded");
    uint128_t incr_stake_acc_per_share = 0;
    uint128_t incr_consensus_acc_per_share = 0;
    uint64_t validator_staking_rewards = staking_rewards;
    uint64_t validator_consensus_rewards = consensus_rewards;
    // calculated reward
    if (staking_rewards > 0 && validator_itr->quantity.amount > 0) {
        validator_staking_rewards = safemath128::muldiv(staking_rewards, validator_itr->commission_rate, RATE_BASE_10000);
        incr_stake_acc_per_share
            = safemath128::muldiv(staking_rewards - validator_staking_rewards, RATE_BASE, validator_itr->quantity.amount);
    }

    if (consensus_rewards > 0 && validator_itr->quantity.amount > 0) {
        validator_consensus_rewards = safemath128::muldiv(consensus_rewards, validator_itr->commission_rate, RATE_BASE_10000);
        incr_consensus_acc_per_share
            = safemath128::muldiv(consensus_rewards - validator_consensus_rewards, RATE_BASE, validator_itr->quantity.amount);
    }

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.stake_acc_per_share += incr_stake_acc_per_share;
        row.consensus_acc_per_share += incr_consensus_acc_per_share;
        row.staking_reward_unclaimed.amount += validator_staking_rewards;
        row.consensus_reward_unclaimed.amount += validator_consensus_rewards;
        row.total_staking_reward.amount += staking_rewards;
        row.staking_reward_balance.amount += staking_rewards;
        row.total_consensus_reward.amount += consensus_rewards;
        row.consensus_reward_balance.amount += consensus_rewards;
        row.latest_reward_block = height;
        row.latest_reward_time = current_time_point();
    });
}

template <typename T, typename C>
void endorse_manage::update_staking_reward(const uint128_t stake_acc_per_share, const uint128_t consensus_acc_per_share,
                                           const uint64_t pre_stake, const uint64_t now_stake, T& _stake, C& stake_itr) {
    uint128_t now_stake_debt = safemath128::muldiv(now_stake, stake_acc_per_share, RATE_BASE);
    check(now_stake_debt <= (uint64_t)-1LL, "endrmng.xsat::update_staking_reward: stake debt overflow");
    uint128_t staking_rewards = safemath128::muldiv(pre_stake, stake_acc_per_share, RATE_BASE) - stake_itr->stake_debt;
    check(staking_rewards <= (uint64_t)-1LL, "endrmng.xsat::update_staking_reward: stake reward overflow");

    uint128_t now_consensus_debt = safemath128::muldiv(now_stake, consensus_acc_per_share, RATE_BASE);
    check(now_consensus_debt <= (uint64_t)-1LL, "endrmng.xsat::update_staking_reward: consensus debt overflow");
    uint128_t consensus_rewards = safemath128::muldiv(pre_stake, consensus_acc_per_share, RATE_BASE) - stake_itr->consensus_debt;
    check(consensus_rewards <= (uint64_t)-1LL, "endrmng.xsat::update_staking_reward: consensus reward overflow");

    _stake.modify(stake_itr, same_payer, [&](auto& row) {
        row.stake_debt = now_stake_debt;
        row.consensus_debt = now_consensus_debt;
        row.staking_reward_unclaimed.amount += staking_rewards;
        row.consensus_reward_unclaimed.amount += consensus_rewards;
        row.quantity.amount = now_stake;
    });
}

//==============================================================  staking xsat
//=========================================================

//@auth xsatstk.xsat
[[eosio::action]]
void endorse_manage::stakexsat(const name& staker, const name& validator, const asset& quantity) {
    require_auth(XSAT_STAKING_CONTRACT);

    auto validator_staking = stake_xsat_without_auth(staker, validator, quantity);

    // log
    endorse_manage::stakexsatlog_action _stakexsatlog(get_self(), {get_self(), "active"_n});
    _stakexsatlog.send(staker, validator, quantity, validator_staking);
}

//@auth xsatstk.xsat
[[eosio::action]]
void endorse_manage::unstakexsat(const name& staker, const name& validator, const asset& quantity) {
    require_auth(XSAT_STAKING_CONTRACT);
    auto validator_staking = unstake_xsat_without_auth(staker, validator, quantity);

    // log
    endorse_manage::unstkxsatlog_action _unstkxsatlog(get_self(), {get_self(), "active"_n});
    _unstkxsatlog.send(staker, validator, quantity, validator_staking);
}

//@auth staker
[[eosio::action]]
void endorse_manage::restakexsat(const name& staker, const name& old_validator, const name& new_validator,
                                 const asset& quantity) {
    require_auth(staker);

    auto old_validator_staking = unstake_xsat_without_auth(staker, old_validator, quantity);
    auto new_validator_staking = stake_xsat_without_auth(staker, new_validator, quantity);

    // log
    endorse_manage::restkxsatlog_action _restkxsatlog(get_self(), {get_self(), "active"_n});
    _restkxsatlog.send(staker, old_validator, new_validator, quantity, old_validator_staking, new_validator_staking);
}

// @auth scope is `evmcaller` evmproxies account
[[eosio::action]]
void endorse_manage::evmstakexsat(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator,
                                  const asset& quantity) {
    require_auth(caller);

    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), caller.value);
    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    evm_proxy_idx.require_find(xsat::utils::compute_id(proxy), "endrmng.xsat::evmstakexsat: [evmproxies] does not exist");

    auto validator_staking = evm_stake_xsat_without_auth(proxy, staker, validator, quantity);

    // log
    endorse_manage::estkxsatlog_action _estkxsatlog(get_self(), {get_self(), "active"_n});
    _estkxsatlog.send(proxy, staker, validator, quantity, validator_staking);
}

// @auth scope is `evmcaller` evmproxies account
[[eosio::action]]
void endorse_manage::evmunstkxsat(const name& caller, const checksum160& proxy, const checksum160& staker, const name& validator,
                                  const asset& quantity) {
    require_auth(caller);

    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), caller.value);
    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    evm_proxy_idx.require_find(xsat::utils::compute_id(proxy), "endrmng.xsat::evmunstkxsat: [evmproxies] does not exist");

    auto validator_staking = evm_unstake_xsat_without_auth(proxy, staker, validator, quantity);

    // log
    endorse_manage::eustkxsatlog_action _eustkxsatlog(get_self(), {get_self(), "active"_n});
    _eustkxsatlog.send(proxy, staker, validator, quantity, validator_staking);
}

// @auth scope is `evmcaller` evmproxies account
[[eosio::action]]
void endorse_manage::evmrestkxsat(const name& caller, const checksum160& proxy, const checksum160& staker,
                                  const name& old_validator, const name& new_validator, const asset& quantity) {
    require_auth(caller);

    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), caller.value);
    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    auto evm_proxy_itr
        = evm_proxy_idx.require_find(xsat::utils::compute_id(proxy), "endrmng.xsat::evmrestkxsat: [evmproxies] does not exist");

    // unstake
    auto old_validator_staking = evm_unstake_xsat_without_auth(proxy, staker, old_validator, quantity);
    auto new_validator_staking = evm_stake_xsat_without_auth(proxy, staker, new_validator, quantity);

    // log
    endorse_manage::erstkxsatlog_action _erstkxsatlog(get_self(), {get_self(), "active"_n});
    _erstkxsatlog.send(proxy, staker, old_validator, new_validator, quantity, old_validator_staking, new_validator_staking);
}

asset endorse_manage::evm_stake_xsat_without_auth(const checksum160& proxy, const checksum160& staker, const name& validator,
                                                  const asset& quantity) {
    check(quantity.amount > 0, "endrmng.xsat::evmstakexsat: quantity must be greater than 0");
    check(quantity.symbol == XSAT_SYMBOL, "endrmng.xsat::evmstakexsat: quantity symbol must be XSAT");
    check(quantity.amount <= XSAT_SUPPLY, "endrmng.xsat::evmstakexsat: quantity must be less than [btc.xsat/BTC] max_supply");

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::evmstakexsat: [validators] does not exists");
    check(!validator_itr->disabled_staking, "endrmng.xsat::evmstakexsat: the current validator's staking status is disabled");

    if (validator_itr->role.has_value()) {

        check(validator_itr->role.value() == 1, "endrmng.xsat::evmstakexsat: only XSAT validator can be staked");
    }

    // check base stake
    auto consens_config = _consensus_config.get_or_default();
    if (validator_itr->stake_address.has_value()) {

        // xsat stake address must be the same as validator's stake address
        check(validator_itr->stake_address.value() == staker,
              "endrmng.xsat::evmstake: only the validator's stake address can stake");
    }

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto staker_itr = evm_staker_idx.find(compute_staking_id(proxy, staker, validator));

    auto stake_quantity = quantity;
    if (staker_itr == evm_staker_idx.end()) {
        auto staking_id = next_staking_id();
        _evm_stake.emplace(get_self(), [&](auto& row) {
            row.id = staking_id;
            row.proxy = proxy;
            row.staker = staker;
            row.validator = validator;
            row.quantity = {0, BTC_SYMBOL};
            row.xsat_quantity = quantity;
            row.total_donated = {0, XSAT_SYMBOL};
            row.staking_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.staking_reward_claimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_claimed = asset{0, XSAT_SYMBOL};
        });
    } else {

        stake_quantity += staker_itr->xsat_quantity;
        evm_staker_idx.modify(staker_itr, same_payer, [&](auto& row) { row.xsat_quantity += quantity; });
    }

    auto active_flag = validator_itr->active_flag.value();
    // V2 check base stake amount
    if (validator_itr->stake_address.has_value() && validator_itr->stake_address.value() == staker) {

        auto config = _consensus_config.get_or_default();
        if (stake_quantity >= config.xsat_base_stake) {

            active_flag = 1;
        } else {

            active_flag = 0;
        }
    }

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.xsat_quantity += quantity;
        row.latest_staking_time = current_time_point();

        row.active_flag = active_flag;
    });

    stat_row stat = _stat.get_or_default();
    stat.xsat_total_staking += quantity;
    _stat.set(stat, get_self());

    return validator_itr->xsat_quantity;
}

asset endorse_manage::evm_unstake_xsat_without_auth(const checksum160& proxy, const checksum160& staker, const name& validator,
                                                    const asset& quantity) {
    check(quantity.amount > 0, "endrmng.xsat::evmunstkxsat: quantity must be greater than 0");
    check(quantity.symbol == XSAT_SYMBOL, "endrmng.xsat::evmunstkxsat: quantity symbol must be XSAT");

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto evm_staker_itr = evm_staker_idx.require_find(
        compute_staking_id(proxy, staker, validator), "endrmng.xsat::evmunstkxsat: [evmstakers] does not exists");
    check(evm_staker_itr->xsat_quantity >= quantity, "endrmng.xsat::evmunstkxsat: insufficient stake");

    auto validator_itr
        = _validator.require_find(evm_staker_itr->validator.value, "endrmng.xsat::evmunstkxsat: [validators] does not exists");

    evm_staker_idx.modify(evm_staker_itr, same_payer, [&](auto& row) { row.xsat_quantity -= quantity; });

    auto active_flag = validator_itr->active_flag.value();
    auto stake_amount = evm_staker_itr->xsat_quantity;
    // V2 check base stake amount
    if (validator_itr->stake_address.has_value() && validator_itr->stake_address.value() == staker) {

        stake_amount -= quantity;
        auto config = _consensus_config.get_or_default();
        if (stake_amount >= config.xsat_base_stake) {

            active_flag = 1;
        } else {

            active_flag = 0;
        }
    }

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.xsat_quantity -= quantity;
        row.latest_staking_time = current_time_point();

        row.active_flag = active_flag;
    });

    stat_row stat = _stat.get_or_default();
    stat.xsat_total_staking -= quantity;
    _stat.set(stat, get_self());

    return validator_itr->xsat_quantity;
}

asset endorse_manage::stake_xsat_without_auth(const name& staker, const name& validator, const asset& quantity) {
    check(quantity.amount > 0, "endrmng.xsat::stakexsat: quantity must be greater than 0");
    check(quantity.symbol == XSAT_SYMBOL, "endrmng.xsat::stakexsat: quantity symbol must be XSAT");

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::stakexsat: [validators] does not exists");
    check(!validator_itr->disabled_staking, "endrmng.xsat::stakexsat: the current validator's staking status is disabled");

    auto native_staker_idx = _native_stake.get_index<"bystakingid"_n>();
    auto stake_itr = native_staker_idx.find(compute_staking_id(staker, validator));

    auto staking_id = next_staking_id();
    if (stake_itr == native_staker_idx.end()) {
        auto stake_itr = _native_stake.emplace(get_self(), [&](auto& row) {
            row.id = staking_id;
            row.staker = staker;
            row.validator = validator;
            row.quantity = asset{0, BTC_SYMBOL};
            row.xsat_quantity = quantity;
            row.total_donated = {0, XSAT_SYMBOL};
            row.staking_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.staking_reward_claimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_claimed = asset{0, XSAT_SYMBOL};
        });
    } else {
        native_staker_idx.modify(stake_itr, same_payer, [&](auto& row) { row.xsat_quantity += quantity; });
    }

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.xsat_quantity += quantity;
        row.latest_staking_time = current_time_point();
    });

    stat_row stat = _stat.get_or_default();
    stat.xsat_total_staking += quantity;
    _stat.set(stat, get_self());

    return validator_itr->xsat_quantity;
}

asset endorse_manage::unstake_xsat_without_auth(const name& staker, const name& validator, const asset& quantity) {
    check(quantity.amount > 0, "endrmng.xsat::unstakexsat: quantity must be greater than 0");
    check(quantity.symbol == XSAT_SYMBOL, "endrmng.xsat::unstakexsat: quantity symbol must be XSAT");

    auto native_staker_idx = _native_stake.get_index<"bystakingid"_n>();
    auto native_staker_itr = native_staker_idx.require_find(
        compute_staking_id(staker, validator), "endrmng.xsat::unstakexsat: [stakers] does not exists");
    check(native_staker_itr->xsat_quantity >= quantity,
          "endrmng.xsat::unstakexsat: the number of unstakes exceeds the staking amount");

    auto validator_itr
        = _validator.require_find(native_staker_itr->validator.value, "endrmng.xsat::unstakexsat: [validators] does not exists");

    native_staker_idx.modify(native_staker_itr, same_payer, [&](auto& row) { row.xsat_quantity -= quantity; });

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.xsat_quantity -= quantity;
        row.latest_staking_time = current_time_point();
    });

    stat_row stat = _stat.get_or_default();
    stat.xsat_total_staking -= quantity;
    _stat.set(stat, get_self());

    return validator_itr->quantity;
}

//============================================================== credit staking btc
//=========================================================

//@auth get_self()
[[eosio::action]]
void endorse_manage::addcrdtproxy(const checksum160& proxy) {
    require_auth(get_self());

    auto credit_proxy_idx = _credit_proxy.get_index<"byproxy"_n>();
    auto credit_proxy_itr = credit_proxy_idx.find(xsat::utils::compute_id(proxy));
    check(credit_proxy_itr == credit_proxy_idx.end(), "endrmng.xsat::addcrdtproxy: proxy already exists");
    _credit_proxy.emplace(get_self(), [&](auto& row) {
        row.id = _credit_proxy.available_primary_key();
        row.proxy = proxy;
    });
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::delcrdtproxy(const checksum160& proxy) {
    require_auth(get_self());

    auto credit_proxy_idx = _credit_proxy.get_index<"byproxy"_n>();
    auto credit_proxy_itr = credit_proxy_idx.require_find(
        xsat::utils::compute_id(proxy), "endrmng.xsat::delcrdtproxy: [evmproxies] does not exist");

    credit_proxy_idx.erase(credit_proxy_itr);
}

// @auth custody.xsat
[[eosio::action]]
void endorse_manage::creditstake(const checksum160& proxy, const checksum160& staker, const name& validator,
                                 const asset& quantity) {
    require_auth(CUSTODY_CONTRACT);

    auto credit_proxy_idx = _credit_proxy.get_index<"byproxy"_n>();
    auto credit_proxy_itr = credit_proxy_idx.require_find(
        xsat::utils::compute_id(proxy), "endrmng.xsat::creditstake: [creditproxy] does not exist");

    check(quantity.symbol == BTC_SYMBOL, "endrmng.xsat::creditstake: quantity symbol must be BTC");
    check(
        quantity.amount <= MIN_BTC_STAKE_FOR_VALIDATOR, "endrmng.xsat::creditstake: quantity must be less than or equal 100 BTC");
    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::creditstake: [validators] does not exists");
    check(!validator_itr->disabled_staking, "endrmng.xsat::creditstake: the current validator's staking status is disabled");

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto stake_itr = evm_staker_idx.find(compute_staking_id(proxy, staker, validator));
    asset old_quantity = {0, BTC_SYMBOL};
    if (stake_itr != evm_staker_idx.end()) {
        old_quantity = stake_itr->quantity;
    }

    if (old_quantity < quantity) {
        asset qualification = old_quantity.amount == 0 ? asset{MIN_BTC_STAKE_FOR_VALIDATOR, BTC_SYMBOL} : asset{0, BTC_SYMBOL};
        asset new_quantity = quantity - old_quantity;

        asset validator_staking, validator_qualification;
        std::tie(validator_staking, validator_qualification)
            = evm_stake_without_auth(proxy, staker, validator, new_quantity, qualification);

        // log
        endorse_manage::evmstakelog_action _evmstakelog(get_self(), {get_self(), "active"_n});
        _evmstakelog.send(proxy, staker, validator, quantity, validator_staking, validator_qualification);
    } else if (old_quantity > quantity) {
        asset qualification = quantity.amount == 0 ? asset{MIN_BTC_STAKE_FOR_VALIDATOR, BTC_SYMBOL} : asset{0, BTC_SYMBOL};
        asset new_quantity = old_quantity - quantity;

        asset validator_staking, validator_qualification;
        std::tie(validator_staking, validator_qualification)
            = evm_unstake_without_auth(proxy, staker, validator, new_quantity, qualification);

        // log
        endorse_manage::evmunstlog_action _evmunstlog(get_self(), {get_self(), "active"_n});
        _evmunstlog.send(proxy, staker, validator, quantity, validator_staking, validator_qualification);
    }
}

//@auth rwddist.xsat
[[eosio::action]]
void endorse_manage::distribute(const uint64_t height, const vector<reward_details_row> rewards) {
    require_auth(REWARD_DISTRIBUTION_CONTRACT);
    check(rewards.size() > 0, "endrmng.xsat::distribute: rewards are empty");
    for (const auto reward : rewards) {
        if (reward.staking_rewards.amount > 0 || reward.consensus_rewards.amount > 0) {
            update_validator_reward(height, reward.validator, reward.staking_rewards.amount, reward.consensus_rewards.amount);
        }
    }
}

[[eosio::on_notify("*::transfer")]]
void endorse_manage::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    // ignore transfers
    if (to != get_self())
        return;

    const name contract = get_first_receiver();
    check(from == REWARD_DISTRIBUTION_CONTRACT, "endrmng.xsat: only transfer from [rwddist.xsat]");
    check(contract == EXSAT_CONTRACT && quantity.symbol == XSAT_SYMBOL, "endrmng.xsat: only transfer [exsat.xsat/XSAT]");
}

void endorse_manage::token_transfer(const name& from, const string& to, const extended_asset& value) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});

    bool is_eos_account = to.size() <= 12;
    if (is_eos_account) {
        transfer.send(from, name(to), value.quantity, "");
    } else {
        transfer.send(from, ERC20_CONTRACT, value.quantity, to);
    }
}

void endorse_manage::token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});
    transfer.send(from, to, value.quantity, memo);
}

uint64_t endorse_manage::next_staking_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.staking_id++;
    _global_id.set(global_id, get_self());
    return global_id.staking_id;
}

///
/// XSAT Staking Support
///
//@auth validator
[[eosio::action]]
void endorse_manage::evmregvldtor(const name& validator, const uint32_t role, const checksum160& stake_addr,
                                  const optional<checksum160>& reward_addr, const optional<uint16_t>& commission_rate) {

    require_auth(validator);

    check(commission_rate <= RATE_BASE_10000,
          "endrmng.xsat::newregvalidator: commission_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    // Check Role
    check(VALIDATOR_ROLE.find(role) != VALIDATOR_ROLE.end(), "endrmng.xsat::newregvalidator: role does not exists");

    check(commission_rate <= RATE_BASE_10000,
          "endrmng.xsat::regvalidator: commission_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    auto validator_itr = _validator.find(validator.value);
    check(validator_itr == _validator.end(), "endrmng.xsat::regvalidator: [validators] already exists");

    auto reward_adddress = stake_addr;
    auto commission = 0;
    // Only BTC validator can be set reward address
    if (role == 0) {

        if (reward_addr.has_value()) {
            reward_adddress = reward_addr.value();
        }

        if (commission_rate.has_value()) {
            commission = commission_rate.value();
        }
    }

    _validator.emplace(get_self(), [&](auto& row) {
        row.owner = validator;
        row.commission_rate = commission;

        row.reward_recipient = ERC20_CONTRACT;
        row.memo = "0x" + xsat::utils::checksum160_to_string(reward_adddress);

        row.quantity = asset{0, BTC_SYMBOL};
        row.xsat_quantity = asset{0, XSAT_SYMBOL};
        row.qualification = {0, BTC_SYMBOL};
        row.donate_rate = 0;
        row.total_donated = {0, XSAT_SYMBOL};
        row.staking_reward_unclaimed = asset{0, XSAT_SYMBOL};
        row.staking_reward_claimed = asset{0, XSAT_SYMBOL};
        row.consensus_reward_unclaimed = asset{0, XSAT_SYMBOL};
        row.consensus_reward_claimed = asset{0, XSAT_SYMBOL};
        row.total_consensus_reward = asset{0, XSAT_SYMBOL};
        row.consensus_reward_balance = asset{0, XSAT_SYMBOL};
        row.total_staking_reward = asset{0, XSAT_SYMBOL};
        row.staking_reward_balance = asset{0, XSAT_SYMBOL};

        row.role = role;
        row.reward_address = reward_adddress;
        row.stake_address = stake_addr;
    });

    // log
    endorse_manage::regvldtorlog_action _validatorlog(get_self(), {get_self(), "active"_n});
    _validatorlog.send(validator, role, stake_addr, reward_addr, commission_rate);
}

//@auth validator
[[eosio::action]]
void endorse_manage::evmconfigvald(const name& validator, const uint16_t commission_rate, const uint16_t donate_rate) {
    require_auth(validator);

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::evmconfigvald: [validators] does not exists");

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.commission_rate = commission_rate;
        row.donate_rate = donate_rate;
    });
}

[[eosio::action]]
void endorse_manage::evmsetstaker(const name& validator, const checksum160& stake_addr) {
    require_auth(validator);

    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat::evmsetstaker: [validators] does not exists");

    check(validator_itr->qualification.amount == 0, "endrmng.xsat::evmsetstaker: the validator's qualification must be 0");

    _validator.modify(validator_itr, same_payer, [&](auto& row) { row.stake_address = stake_addr; });
}

[[eosio::action]]
void endorse_manage::updatexsat(const bool is_open) {
    require_auth(get_self());

    auto config = _consensus_config.get_or_default();

    if (is_open) {

        config.flags |= consensus_config_row::xsat_consensus_mask;
    } else {

        config.flags &= ~consensus_config_row::xsat_consensus_mask;
    }

    _consensus_config.set(config, get_self());
}

//@auth get_self()
// Update xsat base stake
// @param xsat_base_stake - xsat base stake. example 2100 XSAT
// @param btc_base_stake - btc base stake. example 100 BTC
[[eosio::action]]
void endorse_manage::setstakebase(const asset& xsat_base_stake, const asset& btc_base_stake) {
    require_auth(get_self());

    check(xsat_base_stake.symbol == XSAT_SYMBOL, "endrmng.xsat::setstakebase: xsat_base_stake must be XSAT");
    check(btc_base_stake.symbol == BTC_SYMBOL, "endrmng.xsat::setstakebase: btc_base_stake must be BTC");

    check(xsat_base_stake.amount > 0 && xsat_base_stake.amount <= XSAT_SUPPLY,
          "endrmng.xsat::setstakebase: xsat_base_stake must be greater than 0");
    check(btc_base_stake.amount > 0 && BTC_SUPPLY, "endrmng.xsat::setstakebase: btc_base_stake must be greater than 0");

    auto itr = _validator.begin();
    while (itr != _validator.end()) {
        auto active = itr->active_flag;
        if (itr->role.value() == 0) {
            if (itr->qualification >= btc_base_stake) {
                active = 1;
            } else {
                active = 0;
            }
        }

        if (itr->role.value() == 1) {
            if (itr->xsat_quantity >= xsat_base_stake) {
                active = 1;
            } else {
                active = 0;
            }
        }

        _validator.modify(itr, get_self(), [&](auto& row) { row.active_flag = active; });
        itr++;
    }

    auto config = _consensus_config.get_or_default();
    config.xsat_base_stake = xsat_base_stake;
    config.btc_base_stake = btc_base_stake;
    _consensus_config.set(config, get_self());
}

[[eosio::action]]
void endorse_manage::setconsebase(const uint8_t validator_active_vote_count, const uint8_t synchronizer_revote_confirm_count) {
    require_auth(get_self());
    auto config = _consensus_config.get_or_default();
    config.validator_active_vote_count = validator_active_vote_count;
    config.synchronizer_revote_confirm_count = synchronizer_revote_confirm_count;
    _consensus_config.set(config, get_self());
}

[[eosio::action]]
void endorse_manage::upgradev2() {
    require_auth(get_self());

    // 1) Check if the config version is 1
    auto config = _consensus_config.get_or_default();
    check(config.version == 1, "endrmng.xsat::upgratev2: the current version is not 1");

    auto validator_itr = _validator.begin();
    while (validator_itr != _validator.end()) {

        auto active = 0;
        if (validator_itr->qualification >= config.btc_base_stake) {
            active = 1;
        }

        _validator.modify(validator_itr, get_self(), [&](auto& row) {
            // Default validator role is 0 (BTC)
            row.role = 0;
            row.reward_address = xsat::utils::evm_address_to_checksum160(row.memo);
            row.stake_address = xsat::utils::evm_address_to_checksum160(row.memo);
            row.consecutive_vote_count = 0;
            row.latest_consensus_block = 0;
            row.active_flag = active;
        });
        validator_itr++;
    }

    // 4) Update the version from 1 to 2, indicating the upgrade is done
    config.version = 2;
    _consensus_config.set(config, get_self());
}