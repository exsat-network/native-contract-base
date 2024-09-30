#include <xsatstk.xsat/xsatstk.xsat.hpp>
#include <exsat.xsat/exsat.xsat.hpp>
#include <endrmng.xsat/endrmng.xsat.hpp>
#include "../internal/defines.hpp"

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

//@auth get_self()
[[eosio::action]]
void xsat_stake::setstatus(const bool disabled_staking) {
    require_auth(get_self());

    auto config = _config.get_or_default();
    config.disabled_staking = disabled_staking;
    _config.set(config, get_self());
}

//@auth staker
[[eosio::action]]
void xsat_stake::withdraw(const name& staker) {
    require_auth(staker);

    time_point_sec now_time = current_time_point();
    auto _release = release_table(get_self(), staker.value);
    auto release_idx = _release.get_index<"byexpire"_n>();
    auto release_itr = release_idx.lower_bound(0);
    auto end_release_itr = release_idx.upper_bound(now_time.sec_since_epoch());

    check(release_itr != end_release_itr, "xsatstk.xsat::withdraw: there is no expired token that can be withdrawn");

    auto max_row = 100;
    while (release_itr != end_release_itr && max_row--) {
        token_transfer(get_self(), staker, {release_itr->quantity, EXSAT_CONTRACT}, "release");
        release_itr = release_idx.erase(release_itr);
    }
}

//@auth staker
[[eosio::action]]
void xsat_stake::release(const name& staker, const name& validator, const asset& quantity) {
    require_auth(staker);

    auto staking_itr = _staking.require_find(staker.value, "xsatstk.xsat::release: [staking] does not exists");
    check(staking_itr->quantity >= quantity, "xsatstk.xsat::release: the amount released exceeds the amount pledged");

    // sub quantity
    _staking.modify(staking_itr, same_payer, [&](auto& row) {
        row.quantity -= quantity;
    });

    // save release
    auto _release = release_table(get_self(), staker.value);
    auto id = next_release_id();
    _release.emplace(get_self(), [&](auto& row) {
        row.id = id;
        row.quantity = quantity;
        row.expiration_time = current_time_point() + days(STAKE_RELEASE_CYCLE);
    });

    // unstake
    endorse_manage::unstakexsat_action _unstakexsat(ENDORSER_MANAGE_CONTRACT, {get_self(), "active"_n});
    _unstakexsat.send(staker, validator, quantity);
}

[[eosio::on_notify("*::transfer")]]
void xsat_stake::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    // ignore transfers
    if (to != get_self()) return;
    const name contract = get_first_receiver();

    check(contract == EXSAT_CONTRACT && quantity.symbol == XSAT_SYMBOL,
          "xsatstk.xsat: only transfer [exsat.xsat/XSAT]");

    auto validator = xsat::utils::parse_name(memo);
    do_stake(from, validator, quantity);
}

void xsat_stake::do_stake(const name& from, const name& validator, const asset& quantity) {
    check(quantity.amount > 0, "xsatstk.xsat: must transfer positive quantity");
    check(is_account(validator), "xsatstk.xsat: invalid memo, ex: \"<validator>\"");

    auto config = _config.get();
    check(!config.disabled_staking, "xsatstk.xsat: the current xsat staking status is disabled");
    // save stake
    auto staking_itr = _staking.find(from.value);

    if (staking_itr == _staking.end()) {
        _staking.emplace(get_self(), [&](auto& row) {
            row.staker = from;
            row.quantity = quantity;
        });
    } else {
        _staking.modify(staking_itr, same_payer, [&](auto& row) {
            row.quantity += quantity;
        });
    }

    // stake
    endorse_manage::stakexsat_action _stakexsat(ENDORSER_MANAGE_CONTRACT, {get_self(), "active"_n});
    _stakexsat.send(from, validator, quantity);
}

void xsat_stake::token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo) {
    exsat::transfer_action transfer(value.contract, {from, "active"_n});
    transfer.send(from, to, value.quantity, memo);
}

asset xsat_stake::get_balance(const name& owner, const extended_symbol& token) {
    exsat::accounts accounts(token.get_contract(), owner.value);
    auto ac = accounts.find(token.get_symbol().code().raw());
    if (ac == accounts.end()) {
        return {0, token.get_symbol()};
    }
    return ac->balance;
}

uint64_t xsat_stake::next_release_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.release_id++;
    _global_id.set(global_id, get_self());
    return global_id.release_id;
}
