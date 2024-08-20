#include <endrmng.xsat/endrmng.xsat.hpp>
#include <btc.xsat/btc.xsat.hpp>
#include "../internal/safemath.hpp"

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

//@auth get_self()
[[eosio::action]]
void endorse_manage::addwhitelist(const name& type, const name& account) {
    require_auth(get_self());

    check(WHITELIST_TYPES.find(type) != WHITELIST_TYPES.end(), "endrmng.xsat::addwhitelist: type invalid");
    check(is_account(account), "endrmng.xsat::addwhitelist: account does not exist");

    whitelist_table _whitelist(get_self(), type.value);
    auto whitelist_itr = _whitelist.find(account.value);
    check(whitelist_itr == _whitelist.end(), "endrmng.xsat::addwhitelist: whitelist account already exists");

    _whitelist.emplace(get_self(), [&](auto& row) {
        row.account = account;
    });
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::delwhitelist(const name& type, const name& account) {
    require_auth(get_self());

    // check
    check(WHITELIST_TYPES.find(type) != WHITELIST_TYPES.end(), "endrmng.xsat::delwhitelist: [type] is invalid");

    // erase whitelist
    whitelist_table _whitelist(get_self(), type.value);
    auto whitelist_itr
        = _whitelist.require_find(account.value, "endrmng.xsat::delwhitelist: whitelist account does not exist");
    _whitelist.erase(whitelist_itr);
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::addevmproxy(const checksum160& proxy) {
    require_auth(get_self());

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
void endorse_manage::delevmproxy(const checksum160& proxy) {
    require_auth(get_self());

    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    auto evm_proxy_itr = evm_proxy_idx.require_find(xsat::utils::compute_id(proxy),
                                                    "endrmng.xsat::delevmproxy: [evmproxys] does not exist");

    evm_proxy_idx.erase(evm_proxy_itr);
}

//@auth validator
[[eosio::action]]
void endorse_manage::regvalidator(const name& validator, const string& financial_account,
                                  const uint64_t commission_rate) {
    require_auth(validator);
    register_validator({}, validator, financial_account, commission_rate);
}

//@auth proxy
[[eosio::action]]
void endorse_manage::proxyreg(const name& proxy, const name& validator, const string& financial_account,
                              const uint64_t commission_rate) {
    require_auth(proxy);
    whitelist_table _whitelist(get_self(), "proxyreg"_n.value);
    _whitelist.require_find(proxy.value, "endrmng.xsat::proxyreg: the proxy account is not in the whitelist");

    check(is_account(validator), "endrmng.xsat::proxyreg: validator account does not exists");

    register_validator(proxy, validator, financial_account, commission_rate);
}

void endorse_manage::register_validator(const name& proxy, const name& validator, const string& financial_account,
                                        const uint64_t commission_rate) {
    check(commission_rate <= COMMISSION_RATE_BASE,
          "endrmng.xsat::regvalidator: commission_rate must be less than or equal to "
              + std::to_string(COMMISSION_RATE_BASE));
    bool is_eos_address = financial_account.size() <= 12;
    if (is_eos_address) {
        check(is_account(name(financial_account)), "endrmng.xsat::regvalidator: financial account does not exists");
    } else {
        check(xsat::utils::is_valid_evm_address(financial_account),
              "endrmng.xsat::regvalidator: invalid financial account");
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
void endorse_manage::config(const name& validator, const optional<uint64_t>& commission_rate,
                            const optional<string>& financial_account) {
    require_auth(validator);
    check(commission_rate.has_value() || financial_account.has_value(),
          "endrmng.xsat::config: commissionrate and financial account cannot be empty at the same time");
    check(
        !commission_rate.has_value() || *commission_rate <= COMMISSION_RATE_BASE,
        "endrmng.xsat::config: commission_rate must be less than or equal to " + std::to_string(COMMISSION_RATE_BASE));

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
                check(xsat::utils::is_valid_evm_address(*financial_account),
                      "endrmng.xsat::config: invalid financial account");
                row.reward_recipient = ERC20_CONTRACT;
                row.memo = *financial_account;
            }
        }
    });
}

//@auth get_self()
[[eosio::action]]
void endorse_manage::setstatus(const name& validator, const bool disabled_staking) {
    require_auth(get_self());

    auto validator_itr
        = _validator.require_find(validator.value, "endrmng.xsat::setstatus: [validators] does not exists");
    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.disabled_staking = disabled_staking;
    });
}

//@auth staking.xsat
[[eosio::action]]
void endorse_manage::stake(const name& staker, const name& validator, const asset& quantity) {
    require_auth(STAKING_CONTRACT);

    auto validator_staking = stake_without_auth(staker, validator, quantity);

    // log
    endorse_manage::stakelog_action _stakelog(get_self(), {get_self(), "active"_n});
    _stakelog.send(staker, validator, quantity, validator_staking);
}

//@auth staking.xsat
[[eosio::action]]
void endorse_manage::unstake(const name& staker, const name& validator, const asset& quantity) {
    require_auth(STAKING_CONTRACT);
    auto validator_staking = unstake_without_auth(staker, validator, quantity);

    // log
    endorse_manage::unstakelog_action _unstakelog(get_self(), {get_self(), "active"_n});
    _unstakelog.send(staker, validator, quantity, validator_staking);
}

//@auth staker
[[eosio::action]]
void endorse_manage::newstake(const name& staker, const name& old_validator, const name& new_validator,
                              const asset& quantity) {
    require_auth(staker);

    auto old_validator_staking = unstake_without_auth(staker, old_validator, quantity);
    auto new_validator_staking = stake_without_auth(staker, new_validator, quantity);

    // log
    endorse_manage::newstakelog_action _newstakelog(get_self(), {get_self(), "active"_n});
    _newstakelog.send(staker, old_validator, new_validator, quantity, old_validator_staking, new_validator_staking);
}

//@auth staker
[[eosio::action]]
void endorse_manage::claim(const name& staker, const name& validator) {
    require_auth(staker);
    auto native_staker_idx = _native_stake.get_index<"bystakingid"_n>();
    auto native_staker_itr = native_staker_idx.require_find(compute_staking_id(staker, validator),
                                                            "endrmng.xsat::claim: [stakers] does not exists");

    auto validator_itr = _validator.require_find(native_staker_itr->validator.value,
                                                 "endrmng.xsat::claim: [validators] does not exists");
    // update reward
    update_staking_reward(validator_itr->stake_acc_per_share, validator_itr->consensus_acc_per_share,
                          native_staker_itr->quantity.amount, native_staker_itr->quantity.amount, native_staker_idx,
                          native_staker_itr);

    auto staking_reward_unclaimed = native_staker_itr->staking_reward_unclaimed;
    auto consensus_reward_unclaimed = native_staker_itr->consensus_reward_unclaimed;
    auto claimable = staking_reward_unclaimed + consensus_reward_unclaimed;
    check(claimable.amount > 0, "endrmng.xsat::claim: no balance to claim");

    native_staker_idx.modify(native_staker_itr, same_payer, [&](auto& row) {
        row.staking_reward_claimed += staking_reward_unclaimed;
        row.consensus_reward_claimed += consensus_reward_unclaimed;
        row.staking_reward_unclaimed -= staking_reward_unclaimed;
        row.consensus_reward_unclaimed -= consensus_reward_unclaimed;
    });

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.staking_reward_balance -= staking_reward_unclaimed;
        row.consensus_reward_balance -= consensus_reward_unclaimed;
    });

    token_transfer(get_self(), native_staker_itr->staker, {claimable, EXSAT_CONTRACT}, "claim reward");

    endorse_manage::claimlog_action _claimlog(get_self(), {get_self(), "active"_n});
    _claimlog.send(staker, validator, claimable);
}

// @auth caller scope is `evmcaller` whitelist account
[[eosio::action]]
void endorse_manage::evmstake(const name& caller, const checksum160& proxy, const checksum160& staker,
                              const name& validator, const asset& quantity) {
    require_auth(caller);

    whitelist_table _whitelist(get_self(), "evmcaller"_n.value);
    _whitelist.require_find(caller.value, "endrmng.xsat::evmstake: caller is not in the `evmcaller` whitelist");

    auto validator_staking = evm_stake_without_auth(proxy, staker, validator, quantity);

    // log
    endorse_manage::evmstakelog_action _evmstakelog(get_self(), {get_self(), "active"_n});
    _evmstakelog.send(proxy, staker, validator, quantity, validator_staking);
}

// @auth scope is `evmcaller` whitelist account
[[eosio::action]]
void endorse_manage::evmunstake(const name& caller, const checksum160& proxy, const checksum160& staker,
                                const name& validator, const asset& quantity) {
    require_auth(caller);

    whitelist_table _whitelist(get_self(), "evmcaller"_n.value);
    _whitelist.require_find(caller.value, "endrmng.xsat::evmunstake: caller is not in the `evmcaller` whitelist");

    auto validator_staking = evm_unstake_without_auth(proxy, staker, validator, quantity);

    // log
    endorse_manage::evmunstlog_action _evmunstlog(get_self(), {get_self(), "active"_n});
    _evmunstlog.send(proxy, staker, validator, quantity, validator_staking);
}

// @auth scope is `evmcaller` whitelist account
[[eosio::action]]
void endorse_manage::evmnewstake(const name& caller, const checksum160& proxy, const checksum160& staker,
                                 const name& old_validator, const name& new_validator, const asset& quantity) {
    require_auth(caller);

    whitelist_table _whitelist(get_self(), "evmcaller"_n.value);
    _whitelist.require_find(caller.value, "endrmng.xsat::evmnewstake: caller is not in the `evmcaller` whitelist");

    // unstake
    auto old_validator_staking = evm_unstake_without_auth(proxy, staker, old_validator, quantity);
    auto new_validator_staking = evm_stake_without_auth(proxy, staker, new_validator, quantity);

    // log
    endorse_manage::evmnewstlog_action _evmnewstlog(get_self(), {get_self(), "active"_n});
    _evmnewstlog.send(proxy, staker, old_validator, new_validator, quantity, old_validator_staking,
                      new_validator_staking);
}

// @auth scope is `evmcaller` whitelist account
[[eosio::action]]
void endorse_manage::evmclaim(const name& caller, const checksum160& proxy, const checksum160& staker,
                              const name& validator) {
    require_auth(caller);

    whitelist_table _whitelist(get_self(), "evmcaller"_n.value);
    _whitelist.require_find(caller.value, "endrmng.xsat::evmclaim: caller is not in the `evmcaller` whitelist");

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto evm_staker_itr = evm_staker_idx.require_find(compute_staking_id(proxy, staker, validator),
                                                      "endrmng.xsat::evmclaim: [evmstakers] does not exists");

    auto validator_itr = _validator.require_find(evm_staker_itr->validator.value,
                                                 "endrmng.xsat::evmclaim: [validators] does not exists");
    update_staking_reward(validator_itr->stake_acc_per_share, validator_itr->consensus_acc_per_share,
                          evm_staker_itr->quantity.amount, evm_staker_itr->quantity.amount, evm_staker_idx,
                          evm_staker_itr);

    auto staking_reward_unclaimed = evm_staker_itr->staking_reward_unclaimed;
    auto consensus_reward_unclaimed = evm_staker_itr->consensus_reward_unclaimed;
    auto claimable = staking_reward_unclaimed + consensus_reward_unclaimed;
    check(claimable.amount > 0, "endrmng.xsat::evmclaim: no balance to claim");

    evm_staker_idx.modify(evm_staker_itr, same_payer, [&](auto& row) {
        row.staking_reward_claimed += staking_reward_unclaimed;
        row.consensus_reward_claimed += consensus_reward_unclaimed;
        row.staking_reward_unclaimed -= staking_reward_unclaimed;
        row.consensus_reward_unclaimed -= consensus_reward_unclaimed;
    });

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.staking_reward_balance -= staking_reward_unclaimed;
        row.consensus_reward_balance -= consensus_reward_unclaimed;
    });

    token_transfer(get_self(), ERC20_CONTRACT, {claimable, EXSAT_CONTRACT},
                   "0x" + xsat::utils::sha1_to_hex(evm_staker_itr->staker));

    // log
    endorse_manage::evmclaimlog_action _evmclaimlog(get_self(), {get_self(), "active"_n});
    _evmclaimlog.send(proxy, staker, validator, claimable);
}

//@auth validator
[[eosio::action]]
void endorse_manage::vdrclaim(const name& validator) {
    auto validator_itr
        = _validator.require_find(validator.value, "endrmng.xsat::vdrclaim: [validators] does not exists");

    if (validator_itr->reward_recipient == ERC20_CONTRACT) {
        require_auth(EVM_UTIL_CONTRACT);
    } else {
        require_auth(validator_itr->reward_recipient);
    }

    auto staking_reward_unclaimed = validator_itr->staking_reward_unclaimed;
    auto consensus_reward_unclaimed = validator_itr->consensus_reward_unclaimed;
    auto claimable = staking_reward_unclaimed + consensus_reward_unclaimed;
    check(claimable.amount > 0, "endrmng.xsat::vdrclaim: no balance to claim");
    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.staking_reward_claimed += staking_reward_unclaimed;
        row.consensus_reward_claimed += consensus_reward_unclaimed;
        row.staking_reward_unclaimed -= staking_reward_unclaimed;
        row.consensus_reward_unclaimed -= consensus_reward_unclaimed;
        row.consensus_reward_balance -= consensus_reward_unclaimed;
        row.staking_reward_balance -= staking_reward_unclaimed;
    });

    token_transfer(get_self(), validator_itr->reward_recipient, {claimable, EXSAT_CONTRACT}, validator_itr->memo);

    // log
    endorse_manage::vdrclaimlog_action _vdrclaimlog(get_self(), {get_self(), "active"_n});
    if (validator_itr->reward_recipient == ERC20_CONTRACT) {
        _vdrclaimlog.send(validator, validator_itr->memo, claimable);
    } else {
        _vdrclaimlog.send(validator, validator_itr->reward_recipient.to_string(), claimable);
    }
}

asset endorse_manage::evm_stake_without_auth(const checksum160& proxy, const checksum160& staker, const name& validator,
                                             const asset& quantity) {
    check(quantity.amount > 0, "endrmng.xsat::evmstake: quantity must be greater than 0");
    check(quantity.symbol == BTC_SYMBOL, "endrmng.xsat::evmstake: quantity symbol must be BTC");
    check(quantity.amount <= BTC_SUPPLY,
          "endrmng.xsat::evmstake: quantity must be less than [btc.xsat/BTC] max_supply");

    auto evm_proxy_idx = _evm_proxy.get_index<"byproxy"_n>();
    auto evm_proxy_itr = evm_proxy_idx.require_find(xsat::utils::compute_id(proxy),
                                                    "endrmng.xsat::evmstake: [evmproxys] does not exist");
    auto validator_itr
        = _validator.require_find(validator.value, "endrmng.xsat::evmstake: [validators] does not exists");
    check(!validator_itr->disabled_staking,
          "endrmng.xsat::evmstake: the current validator's staking status is disabled");

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto stake_itr = evm_staker_idx.find(compute_staking_id(proxy, staker, validator));
    if (stake_itr == evm_staker_idx.end()) {
        auto staking_id = next_staking_id();
        auto stake_itr = _evm_stake.emplace(get_self(), [&](auto& row) {
            row.id = staking_id;
            row.proxy = proxy;
            row.staker = staker;
            row.validator = validator;
            row.quantity = {0, BTC_SYMBOL};
            row.staking_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.staking_reward_claimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_claimed = asset{0, XSAT_SYMBOL};
        });

        staking_change(validator_itr, _evm_stake, stake_itr, quantity);
    } else {
        staking_change(validator_itr, evm_staker_idx, stake_itr, quantity);
    }
    return validator_itr->quantity;
}

asset endorse_manage::evm_unstake_without_auth(const checksum160& proxy, const checksum160& staker,
                                               const name& validator, const asset& quantity) {
    check(quantity.amount > 0, "endrmng.xsat::evmunstake: quantity must be greater than 0");
    check(quantity.symbol == BTC_SYMBOL, "endrmng.xsat::evmunstake: quantity symbol must be BTC");

    auto evm_staker_idx = _evm_stake.get_index<"bystakingid"_n>();
    auto evm_staker_itr = evm_staker_idx.require_find(compute_staking_id(proxy, staker, validator),
                                                      "endrmng.xsat::evmunstake: [evmstakers] does not exists");
    check(evm_staker_itr->quantity >= quantity, "endrmng.xsat::evmunstake: insufficient stake");

    auto validator_itr = _validator.require_find(evm_staker_itr->validator.value,
                                                 "endrmng.xsat::evmunstake: [validators] does not exists");

    staking_change(validator_itr, evm_staker_idx, evm_staker_itr, -quantity);
    return validator_itr->quantity;
}

asset endorse_manage::stake_without_auth(const name& staker, const name& validator, const asset& quantity) {
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

            row.staking_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.staking_reward_claimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_unclaimed = asset{0, XSAT_SYMBOL};
            row.consensus_reward_claimed = asset{0, XSAT_SYMBOL};
        });
        staking_change(validator_itr, _native_stake, stake_itr, quantity);
    } else {
        staking_change(validator_itr, native_staker_idx, stake_itr, quantity);
    }
    return validator_itr->quantity;
}

asset endorse_manage::unstake_without_auth(const name& staker, const name& validator, const asset& quantity) {
    check(quantity.amount > 0, "endrmng.xsat::unstake: quantity must be greater than 0");
    check(quantity.symbol == BTC_SYMBOL, "endrmng.xsat::unstake: quantity symbol must be BTC");

    auto native_staker_idx = _native_stake.get_index<"bystakingid"_n>();
    auto native_staker_itr = native_staker_idx.require_find(compute_staking_id(staker, validator),
                                                            "endrmng.xsat::unstake: [stakers] does not exists");
    check(native_staker_itr->quantity >= quantity,
          "endrmng.xsat::unstake: the number of unstakes exceeds the staking amount");

    auto validator_itr = _validator.require_find(native_staker_itr->validator.value,
                                                 "endrmng.xsat::unstake: [validators] does not exists");

    staking_change(validator_itr, native_staker_idx, native_staker_itr, -quantity);
    return validator_itr->quantity;
}

template <typename T, typename C>
void endorse_manage::staking_change(validator_table::const_iterator& validator_itr, T& _stake, C& stake_itr,
                                    const asset& quantity) {
    // update reward
    auto now_amount_for_validator = validator_itr->quantity + quantity;
    auto pre_amount_for_staker = stake_itr->quantity;
    auto now_amount_for_staker = stake_itr->quantity + quantity;

    _validator.modify(validator_itr, same_payer, [&](auto& row) {
        row.quantity = now_amount_for_validator;
        row.latest_staking_time = current_time_point();
    });

    stat_row stat = _stat.get_or_default();
    stat.total_staking += quantity;
    _stat.set(stat, get_self());

    update_staking_reward(validator_itr->stake_acc_per_share, validator_itr->consensus_acc_per_share,
                          pre_amount_for_staker.amount, now_amount_for_staker.amount, _stake, stake_itr);
}

void endorse_manage::update_validator_reward(const uint64_t height, const name& validator,
                                             const uint64_t staking_rewards, const uint64_t consensus_rewards) {
    auto validator_itr = _validator.require_find(validator.value, "endrmng.xsat: [validators] does not exists");
    check(validator_itr->latest_reward_block < height, "endrmng.xsat: the block height has been rewarded");
    uint128_t incr_stake_acc_per_share = 0;
    uint128_t incr_consensus_acc_per_share = 0;
    uint64_t validator_staking_rewards = staking_rewards;
    uint64_t validator_consensus_rewards = consensus_rewards;
    // calculated reward
    if (staking_rewards > 0 && validator_itr->quantity.amount > 0) {
        validator_staking_rewards
            = safemath128::muldiv(staking_rewards, validator_itr->commission_rate, COMMISSION_RATE_BASE);
        incr_stake_acc_per_share = safemath128::muldiv(staking_rewards - validator_staking_rewards, RATE_BASE,
                                                       validator_itr->quantity.amount);
    }

    if (consensus_rewards > 0 && validator_itr->quantity.amount > 0) {
        validator_consensus_rewards
            = safemath128::muldiv(consensus_rewards, validator_itr->commission_rate, COMMISSION_RATE_BASE);
        incr_consensus_acc_per_share = safemath128::muldiv(consensus_rewards - validator_consensus_rewards, RATE_BASE,
                                                           validator_itr->quantity.amount);
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
                                           const uint64_t pre_stake, const uint64_t now_stake, T& _stake,
                                           C& stake_itr) {
    uint128_t now_stake_debt = safemath128::muldiv(now_stake, stake_acc_per_share, RATE_BASE);
    check(now_stake_debt <= (uint64_t)-1LL, "endrmng.xsat::update_staking_reward: stake debt overflow");
    uint128_t staking_rewards = safemath128::muldiv(pre_stake, stake_acc_per_share, RATE_BASE) - stake_itr->stake_debt;
    check(staking_rewards <= (uint64_t)-1LL, "endrmng.xsat::update_staking_reward: stake reward overflow");

    uint128_t now_consensus_debt = safemath128::muldiv(now_stake, consensus_acc_per_share, RATE_BASE);
    check(now_consensus_debt <= (uint64_t)-1LL, "endrmng.xsat::update_staking_reward: consensus debt overflow");
    uint128_t consensus_rewards
        = safemath128::muldiv(pre_stake, consensus_acc_per_share, RATE_BASE) - stake_itr->consensus_debt;
    check(consensus_rewards <= (uint64_t)-1LL, "endrmng.xsat::update_staking_reward: consensus reward overflow");

    _stake.modify(stake_itr, same_payer, [&](auto& row) {
        row.stake_debt = now_stake_debt;
        row.consensus_debt = now_consensus_debt;
        row.staking_reward_unclaimed.amount += staking_rewards;
        row.consensus_reward_unclaimed.amount += consensus_rewards;
        row.quantity.amount = now_stake;
    });
}

//@auth rwddist.xsat
[[eosio::action]]
void endorse_manage::distribute(const uint64_t height, const vector<reward_details_row> rewards) {
    require_auth(REWARD_DISTRIBUTION_CONTRACT);
    check(rewards.size() > 0, "endrmng.xsat::distribute: rewards are empty");
    for (const auto reward : rewards) {
        update_validator_reward(height, reward.validator, reward.staking_rewards.amount,
                                reward.consensus_rewards.amount);
    }
}

[[eosio::on_notify("*::transfer")]]
void endorse_manage::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    // ignore transfers
    if (to != get_self()) return;

    const name contract = get_first_receiver();
    check(from == REWARD_DISTRIBUTION_CONTRACT, "endrmng.xsat: only transfer from [rwddist.xsat]");
    check(contract == EXSAT_CONTRACT && quantity.symbol == XSAT_SYMBOL,
          "endrmng.xsat: only transfer [exsat.xsat/XSAT]");
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