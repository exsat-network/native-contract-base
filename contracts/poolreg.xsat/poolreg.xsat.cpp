#include <poolreg.xsat/poolreg.xsat.hpp>
#include <rescmng.xsat/rescmng.xsat.hpp>
#include <btc.xsat/btc.xsat.hpp>
#include "../internal/defines.hpp"

//@auth blksync.xsat
[[eosio::action]]
void pool::updateheight(const name& synchronizer, const uint64_t latest_produced_block_height,
                        const std::vector<string>& miners) {
    require_auth(BLOCK_SYNC_CONTRACT);

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
        });
    } else {
        if (synchronizer_itr->latest_produced_block_height < latest_produced_block_height) {
            _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
                row.latest_produced_block_height = latest_produced_block_height;
            });
        }
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

    auto claimable = synchronizer_itr->unclaimed;
    _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
        row.unclaimed.amount = 0;
        row.claimed += claimable;
    });

    token_transfer(get_self(), synchronizer_itr->reward_recipient, extended_asset{claimable, EXSAT_CONTRACT},
                   synchronizer_itr->memo);

    // log
    pool::claimlog_action _claimlog(get_self(), {get_self(), "active"_n});
    if (synchronizer_itr->reward_recipient == ERC20_CONTRACT) {
        _claimlog.send(synchronizer, synchronizer_itr->memo, claimable);
    } else {
        _claimlog.send(synchronizer, synchronizer_itr->reward_recipient.to_string(), claimable);
    }
}

//@auth synchronizer
[[eosio::action]]
void pool::buyslot(const name& synchronizer, const name& receiver, const uint16_t num_slots) {
    require_auth(synchronizer);
    check(num_slots > 0, "recsmng.xsat::buyslot: num_slots must be greater than 0");

    auto synchronizer_itr
        = _synchronizer.require_find(receiver.value, "recsmng.xsat::buyslot: [synchronizer] does not exists");
    _synchronizer.modify(synchronizer_itr, same_payer, [&](auto& row) {
        row.num_slots += num_slots;
    });

    // fee deduction
    resource_management::pay_action pay(RESOURCE_MANAGE_CONTRACT, {get_self(), "active"_n});
    pay.send(0, checksum256(), synchronizer, BUY_SLOT, num_slots);
}

[[eosio::on_notify("*::transfer")]]
void pool::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    // ignore transfers
    if (to != get_self()) return;

    const name contract = get_first_receiver();
    check(from == REWARD_DISTRIBUTION_CONTRACT, "poolreg.xsat: only transfer from [rwddist.xsat]");
    check(contract == EXSAT_CONTRACT && quantity.symbol == XSAT_SYMBOL,
          "poolreg.xsat: only transfer [exsat.xsat/XSAT]");
    auto parts = xsat::utils::split(memo, ",");
    auto INVALID_MEMO = "poolreg.xsat: invalid memo ex: \"<synchronizer><height>\"";
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
    for (const auto miner : miners) {
        auto miner_idx = _miner.get_index<"byminer"_n>();
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

void pool::token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});
    transfer.send(from, to, value.quantity, memo);
}
