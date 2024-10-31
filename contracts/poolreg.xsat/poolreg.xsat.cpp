#include <poolreg.xsat/poolreg.xsat.hpp>
#include <rescmng.xsat/rescmng.xsat.hpp>
#include <btc.xsat/btc.xsat.hpp>
#include <bitcoin/script/address.hpp>
#include "../internal/defines.hpp"

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

//@auth get_self()
[[eosio::action]]
void pool::setdonateacc(const string& donation_account, const uint16_t min_donate_rate) {
    require_auth(get_self());

    bool is_eos_address = donation_account.size() <= 12;
    if (is_eos_address) {
        check(is_account(name(donation_account)), "poolreg.xsat::setdonateacc: donation account does not exists");
    } else {
        check(xsat::utils::is_valid_evm_address(donation_account),
              "poolreg.xsat::setdonateacc: invalid donation account");
    }
    check(
        min_donate_rate <= RATE_BASE_10000,
        "poolreg.xsat::setdonateacc: min_donate_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    auto config = _config.get_or_default();
    config.donation_account = donation_account;
    config.min_donate_rate = min_donate_rate;
    _config.set(config, get_self());
}

//@auth utxomng.xsat
[[eosio::action]]
void pool::updateheight(const name& synchronizer, const uint64_t latest_produced_block_height,
                        const std::vector<string>& miners) {
    require_auth(UTXO_MANAGE_CONTRACT);

    check(is_account(synchronizer), "poolreg.xsat::updateheight: synchronizer does not exists");

    auto synchronizer_itr = _synchronizer.find(synchronizer.value);
    if (synchronizer_itr == _synchronizer.end()) {
        synchronizer_itr = _synchronizer.emplace(get_self(), [&](auto& row) {
            row.synchronizer = synchronizer;
            row.reward_recipient = synchronizer;
            row.num_slots = DEFAULT_NUM_SLOTS;
            row.produced_block_limit = DEFAULT_PRODUCTED_BLOCK_LIMIT;
            row.latest_produced_block_height = latest_produced_block_height;
            row.claimed = asset{0, XSAT_SYMBOL};
            row.unclaimed = asset{0, XSAT_SYMBOL};
            row.donate_rate = 0;
            row.total_donated = asset{0, XSAT_SYMBOL};
        });
    } else {
        if (synchronizer_itr->latest_produced_block_height >= latest_produced_block_height) {
            return;
        }
        _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
            row.latest_produced_block_height = latest_produced_block_height;
        });
    }

    save_miners(synchronizer, miners);

    // log
    pool::poollog_action _poollog(get_self(), {get_self(), "active"_n});

    if (synchronizer_itr->reward_recipient == ERC20_CONTRACT) {
        _poollog.send(synchronizer, latest_produced_block_height, synchronizer_itr->memo);
    } else {
        _poollog.send(synchronizer, latest_produced_block_height, synchronizer_itr->reward_recipient.to_string());
    }
}

//@auth get_self()
[[eosio::action]]
void pool::initpool(const name& synchronizer, const uint64_t latest_produced_block_height,
                    const string& financial_account, const std::vector<string>& miners) {
    require_auth(get_self());

    check(is_account(synchronizer), "poolreg.xsat::initpool: synchronizer does not exists");

    for (const auto& miner : miners) {
        check(bitcoin::IsValid(miner, CHAIN_PARAMS), "poolreg.xsat::initpool: invalid miner [\"" + miner + "\"]");
    }

    bool is_eos_address = financial_account.size() <= 12;
    if (is_eos_address) {
        check(is_account(name(financial_account)), "poolreg.xsat::initpool: financial account does not exists");
    } else {
        check(xsat::utils::is_valid_evm_address(financial_account),
              "poolreg.xsat::initpool: invalid financial account");
    }

    name reward_recipient;
    string memo;
    if (is_eos_address) {
        reward_recipient = name(financial_account);
    } else {
        reward_recipient = ERC20_CONTRACT;
        memo = financial_account;
    }

    auto synchronizer_itr = _synchronizer.find(synchronizer.value);
    if (synchronizer_itr == _synchronizer.end()) {
        synchronizer_itr = _synchronizer.emplace(get_self(), [&](auto& row) {
            row.synchronizer = synchronizer;
            row.num_slots = DEFAULT_NUM_SLOTS;
            row.produced_block_limit = DEFAULT_PRODUCTED_BLOCK_LIMIT;
            row.latest_produced_block_height = latest_produced_block_height;
            row.claimed = asset{0, XSAT_SYMBOL};
            row.unclaimed = asset{0, XSAT_SYMBOL};
            row.reward_recipient = reward_recipient;
            row.donate_rate = 0;
            row.total_donated = asset{0, XSAT_SYMBOL};
            row.memo = memo;
        });
    } else {
        if (synchronizer_itr->latest_produced_block_height < latest_produced_block_height) {
            _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
                row.latest_produced_block_height = latest_produced_block_height;
                row.reward_recipient = reward_recipient;
                row.memo = memo;
            });
        }
    }
    save_miners(synchronizer, miners);

    // log
    pool::poollog_action _poollog(get_self(), {get_self(), "active"_n});
    _poollog.send(synchronizer, latest_produced_block_height, financial_account);
}

//@auth get_self()
[[eosio::action]]
void pool::delpool(const name& synchronizer) {
    require_auth(get_self());

    auto synchronizer_itr
        = _synchronizer.require_find(synchronizer.value, "poolreg.xsat::delpool: [synchronizer] does not exists");
    _synchronizer.erase(synchronizer_itr);

    // erase miners
    auto miner_idx = _miner.get_index<"bysyncer"_n>();
    auto miner_itr = miner_idx.lower_bound(synchronizer.value);
    auto end_miner = miner_idx.upper_bound(synchronizer.value);
    while (miner_itr != end_miner) {
        miner_itr = miner_idx.erase(miner_itr);
    }

    // log
    pool::delpoollog_action _delpoollog(get_self(), {get_self(), "active"_n});
    _delpoollog.send(synchronizer);
}

//@auth get_self()
[[eosio::action]]
void pool::unbundle(const uint64_t id) {
    require_auth(get_self());

    auto miner_itr = _miner.require_find(id, "poolreg.xsat::unbundle: [miners] does not exists");

    _miner.erase(miner_itr);
}

//@auth get_self()
[[eosio::action]]
void pool::config(const name& synchronizer, const uint16_t produced_block_limit) {
    require_auth(get_self());

    auto synchronizer_itr
        = _synchronizer.require_find(synchronizer.value, "poolreg.xsat::config: [synchronizer] does not exists");

    _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
        row.produced_block_limit = produced_block_limit;
    });
}

//@auth synchronizer
[[eosio::action]]
void pool::setfinacct(const name& synchronizer, const string& financial_account) {
    require_auth(synchronizer);

    auto synchronizer_itr
        = _synchronizer.require_find(synchronizer.value, "poolreg.xsat::setfinacct: [synchronizer] does not exists");
    bool is_eos_address = financial_account.size() <= 12;
    if (is_eos_address) {
        check(is_account(name(financial_account)), "poolreg.xsat::setfinacct: financial account does not exists");
    } else {
        check(xsat::utils::is_valid_evm_address(financial_account),
              "poolreg.xsat::setfinacct: invalid financial account");
    }

    _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
        if (is_eos_address) {
            row.reward_recipient = name(financial_account);
            row.memo = "";
        } else {
            row.reward_recipient = ERC20_CONTRACT;
            row.memo = financial_account;
        }
    });

    // log
    pool::poollog_action _poollog(get_self(), {get_self(), "active"_n});
    if (synchronizer_itr->reward_recipient == ERC20_CONTRACT) {
        _poollog.send(synchronizer, synchronizer_itr->latest_produced_block_height, synchronizer_itr->memo);
    } else {
        _poollog.send(synchronizer, synchronizer_itr->latest_produced_block_height,
                      synchronizer_itr->reward_recipient.to_string());
    }
}

[[eosio::action]]
void pool::setdonate(const name& synchronizer, const uint16_t donate_rate) {
    require_auth(synchronizer);

    check(donate_rate <= RATE_BASE_10000,
          "poolreg.xsat::setdonate: donate_rate must be less than or equal to " + std::to_string(RATE_BASE_10000));

    auto synchronizer_itr
        = _synchronizer.require_find(synchronizer.value, "poolreg.xsat::setdonate: [synchronizer] does not exists");

    _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
        row.donate_rate = donate_rate;
    });

    // log
    pool::setdonatelog_action _setdonatelog(get_self(), {get_self(), "active"_n});
    _setdonatelog.send(synchronizer, donate_rate);
}

//@auth synchronizer->reward_recipient or evmutil.xsat
[[eosio::action]]
void pool::claim(const name& synchronizer) {
    auto synchronizer_itr = _synchronizer.find(synchronizer.value);
    check(synchronizer_itr != _synchronizer.end() && synchronizer_itr->unclaimed.amount > 0,
          "poolreg.xsat::claim: no balance to claim");
    check(synchronizer_itr->reward_recipient.value != 0,
          "poolreg.xsat::claim: please set up a financial account first");

    // auth
    if (synchronizer_itr->reward_recipient == ERC20_CONTRACT) {
        require_auth(EVM_UTIL_CONTRACT);
    } else {
        require_auth(synchronizer_itr->reward_recipient);
    }

    auto config = _config.get();
    auto donate_rate = std::max(config.min_donate_rate.value_or(uint16_t(0)), synchronizer_itr->donate_rate);

    auto claimable = synchronizer_itr->unclaimed;
    auto donated_amount = claimable * donate_rate / RATE_BASE_10000;
    auto to_synchronizer = claimable - donated_amount;
    _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
        row.unclaimed.amount = 0;
        row.claimed += claimable;
        row.total_donated += donated_amount;
    });

    // transfer donate
    if (donated_amount.amount > 0) {
        token_transfer(get_self(), config.donation_account, extended_asset{donated_amount, EXSAT_CONTRACT});

        auto stat = _stat.get_or_default();
        stat.xsat_total_donated += donated_amount;
        _stat.set(stat, get_self());
    }

    // transfer reward
    if (to_synchronizer.amount > 0) {
        token_transfer(get_self(), synchronizer_itr->reward_recipient, extended_asset{to_synchronizer, EXSAT_CONTRACT},
                       synchronizer_itr->memo);
    }

    // log

    string reward_recipient = synchronizer_itr->reward_recipient == ERC20_CONTRACT
                                  ? synchronizer_itr->memo
                                  : synchronizer_itr->reward_recipient.to_string();
    pool::claimlog_action _claimlog(get_self(), {get_self(), "active"_n});
    _claimlog.send(synchronizer, reward_recipient, claimable, donated_amount, synchronizer_itr->total_donated);
}

//@auth synchronizer
[[eosio::action]]
void pool::buyslot(const name& synchronizer, const name& receiver, const uint16_t num_slots) {
    require_auth(synchronizer);
    check(num_slots > 0, "recsmng.xsat::buyslot: num_slots must be greater than 0");

    auto synchronizer_itr
        = _synchronizer.require_find(receiver.value, "recsmng.xsat::buyslot: [synchronizer] does not exists");

    check(synchronizer_itr->num_slots + num_slots <= MAX_NUM_SLOTS,
          "recsmng.xsat::buyslot: the total number of slots purchased cannot exceed [1000]");
    _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
        row.num_slots += num_slots;
    });

    // fee deduction
    resource_management::pay_action pay(RESOURCE_MANAGE_CONTRACT, {get_self(), "active"_n});
    pay.send(0, ZERO_HASH, synchronizer, BUY_SLOT, num_slots);
}

[[eosio::on_notify("*::transfer")]]
void pool::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    // ignore transfers
    if (to != get_self())
        return;

    const name contract = get_first_receiver();
    check(from == REWARD_DISTRIBUTION_CONTRACT, "poolreg.xsat: only transfer from [rwddist.xsat]");
    check(contract == EXSAT_CONTRACT && quantity.symbol == XSAT_SYMBOL,
          "poolreg.xsat: only transfer [exsat.xsat/XSAT]");
    auto parts = xsat::utils::split(memo, ",");
    auto INVALID_MEMO = "poolreg.xsat: invalid memo ex: \"<synchronizer>,<height>\"";
    check(parts.size() == 2, INVALID_MEMO);
    auto synchronizer = xsat::utils::parse_name(parts[0]);
    check(xsat::utils::is_digit(parts[1]), INVALID_MEMO);
    auto height = std::stoll(parts[1]);

    auto synchronizer_itr = _synchronizer.require_find(synchronizer.value, INVALID_MEMO);
    check(synchronizer_itr->latest_reward_block < height,
          "poolreg.xsat: this height reward has already been distributed");
    _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
        row.latest_reward_block = height;
        row.latest_reward_time = current_time_point();
        row.unclaimed += quantity;
    });
}

void pool::save_miners(const name& synchronizer, const vector<string>& miners) {
    auto miner_idx = _miner.get_index<"byminer"_n>();
    for (const auto miner : miners) {
        auto miner_itr = miner_idx.find(xsat::utils::hash(miner));
        if (miner_itr == miner_idx.end()) {
            _miner.emplace(get_self(), [&](auto& row) {
                row.id = _miner.available_primary_key();
                row.synchronizer = synchronizer;
                row.miner = miner;
            });
        }
    }
}

void pool::token_transfer(const name& from, const string& to, const extended_asset& value) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});

    bool is_eos_account = to.size() <= 12;
    if (is_eos_account) {
        transfer.send(from, name(to), value.quantity, "");
    } else {
        transfer.send(from, ERC20_CONTRACT, value.quantity, to);
    }
}

void pool::token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});
    transfer.send(from, to, value.quantity, memo);
}
