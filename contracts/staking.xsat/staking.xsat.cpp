#include <staking.xsat/staking.xsat.hpp>
#include <btc.xsat/btc.xsat.hpp>
#include <endrmng.xsat/endrmng.xsat.hpp>
#include "../internal/defines.hpp"

//@auth get_self()
[[eosio::action]]
void stake::addtoken(const extended_symbol& token) {
    require_auth(get_self());

    auto idx = _token.get_index<"bytoken"_n>();
    auto itr = idx.find(xsat::utils::compute_id(token));
    check(itr == idx.end(), "staking.xsat::addtoken: token already exists");

    // check token supply
    btc::get_supply(token.get_contract(), token.get_symbol().code());
    auto id = _token.available_primary_key();
    if (id == 0) {
        id = 1;
    }
    _token.emplace(get_self(), [&](auto& row) {
        row.id = id;
        row.token = token;
    });
}

//@auth get_self()
[[eosio::action]]
void stake::deltoken(const uint64_t id) {
    require_auth(get_self());
    auto itr = _token.require_find(id, "staking.xsat::deltoken: [tokens] does not exists");
    asset balance = get_balance(get_self(), itr->token);
    check(balance.amount == 0, "staking.xsat::deltoken: there is currently a balance that has not been withdrawn");

    _token.erase(itr);
}

//@auth get_self()
[[eosio::action]]
void stake::setstatus(const uint64_t id, const bool disabled_staking) {
    require_auth(get_self());

    auto itr = _token.require_find(id, "staking.xsat::config: [tokens] does not exists");
    _token.modify(itr, same_payer, [&](auto& row) {
        row.disabled_staking = disabled_staking;
    });
}

//@auth staker
[[eosio::action]]
void stake::withdraw(const name& staker) {
    require_auth(staker);

    time_point_sec now_time = current_time_point();
    auto _release = release_table(get_self(), staker.value);
    auto release_idx = _release.get_index<"byexpire"_n>();
    auto latest_itr = release_idx.upper_bound(now_time.sec_since_epoch());
    auto release_itr = release_idx.lower_bound(0);

    check(release_itr != latest_itr, "staking.xsat::withdraw: there is no expired token that can be withdrawn");

    auto max_row = 30;
    while (release_itr != latest_itr && max_row--) {
        token_transfer(get_self(), staker, release_itr->quantity, "release");
        release_idx.erase(release_itr);
        release_itr = release_idx.lower_bound(0);
    }
}

//@auth staker
[[eosio::action]]
void stake::release(const uint64_t staking_id, const name& staker, const name& validator, const asset& quantity) {
    require_auth(staker);

    staking_table _stake(get_self(), staker.value);
    auto stake_itr = _stake.require_find(staking_id, "staking.xsat::release: [staking] does not exists");
    check(stake_itr->quantity.quantity >= quantity,
          "staking.xsat::release: the amount released exceeds the amount pledged");

    // sub quantity
    _stake.modify(stake_itr, same_payer, [&](auto& row) {
        row.quantity.quantity -= quantity;
    });

    // save release
    auto _release = release_table(get_self(), staker.value);
    auto id = next_release_id();
    _release.emplace(get_self(), [&](auto& row) {
        row.id = id;
        row.quantity = {quantity, stake_itr->quantity.contract};
        row.expiration_time = current_time_point() + days(STAKE_RELEASE_CYCLE);
    });

    // unstake
    endorse_manage::unstake_action _unstake(ENDORSER_MANAGE_CONTRACT, {get_self(), "active"_n});
    _unstake.send(staker, validator, asset{quantity.amount, BTC_SYMBOL});
}

[[eosio::on_notify("*::transfer")]]
void stake::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    // ignore transfers
    if (to != get_self()) return;

    const name contract = get_first_receiver();
    auto validator = xsat::utils::parse_name(memo);
    do_stake(from, validator, {quantity, contract});
}

void stake::do_stake(const name& from, const name& validator, const extended_asset& ext_in) {
    check(ext_in.quantity.amount > 0, "staking.xsat: must transfer positive quantity");
    check(is_account(validator), "staking.xsat: invalid memo, ex: \"<validator>\"");

    auto token_id = xsat::utils::compute_id(ext_in.get_extended_symbol());
    auto token_idx = _token.get_index<"bytoken"_n>();
    auto token_itr = token_idx.require_find(token_id, "staking.xsat: token does not support staking");
    check(!token_itr->disabled_staking, "staking.xsat: this token is prohibited from staking");

    // save stake
    staking_table stake(get_self(), from.value);
    auto stake_idx = stake.get_index<"bytoken"_n>();
    auto stake_itr = stake_idx.find(token_id);

    if (stake_itr == stake_idx.end()) {
        stake.emplace(get_self(), [&](auto& row) {
            row.id = next_staking_id();
            row.quantity = ext_in;
        });
    } else {
        stake_idx.modify(stake_itr, same_payer, [&](auto& row) {
            row.quantity += ext_in;
        });
    }

    // stake
    endorse_manage::stake_action _stake(ENDORSER_MANAGE_CONTRACT, {get_self(), "active"_n});
    _stake.send(from, validator, asset{ext_in.quantity.amount, BTC_SYMBOL});
}

void stake::token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});
    transfer.send(from, to, value.quantity, memo);
}

uint64_t stake::next_release_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.release_id++;
    _global_id.set(global_id, get_self());
    return global_id.release_id;
}

uint64_t stake::next_staking_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.staking_id++;
    _global_id.set(global_id, get_self());
    return global_id.staking_id;
}

asset stake::get_balance(const name& owner, const extended_symbol& token) {
    btc::accounts accounts(token.get_contract(), owner.value);
    auto ac = accounts.find(token.get_symbol().code().raw());
    if (ac == accounts.end()) {
        return {0, token.get_symbol()};
    }
    return ac->balance;
}