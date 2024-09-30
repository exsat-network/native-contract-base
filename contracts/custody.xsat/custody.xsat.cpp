#include <custody.xsat/custody.xsat.hpp>

#ifdef DEBUG
#include <bitcoin/script/address.hpp>
#include "./src/debug.hpp"
#endif

[[eosio::action]]
void custody::addcustody(const checksum160 staker, const checksum160 proxy, const name validator, const optional<string> btc_address, optional<vector<uint8_t>> scriptpubkey) {
    require_auth(get_self());
    check(is_account(validator), "custody.xsat::addcustody: validator does not exists");
    check(proxy != checksum160(), "custody.xsat::addcustody: proxy cannot be empty");
    check(xsat::utils::is_proxy_valid(proxy), "custody.xsat::addcustody: proxy is not valid");
    check(btc_address.has_value() || scriptpubkey.has_value(), "custody.xsat::addcustody: btc_address and scriptpubkey cannot be empty at the same time");

    endorse_manage::evm_proxy_table _proxy(ENDORSER_MANAGE_CONTRACT, CUSTODY_CONTRACT.value);
    auto proxy_hash = xsat::utils::compute_id(proxy);
    auto proxy_idx = _proxy.get_index<"byproxy"_n>();
    proxy_idx.require_find(proxy_hash, "custody.xsat::addcustody: proxy does not exists");

    auto staker_idx = _custody.get_index<"bystaker"_n>();
    auto staker_itr = staker_idx.find(xsat::utils::compute_id(staker));
    check(staker_itr == staker_idx.end(), "custody.xsat::addcustody: staker address already exists");

    if (btc_address.has_value() && !btc_address->empty()) {
        scriptpubkey = bitcoin::utility::address_to_scriptpubkey(*btc_address);
    }
    check(scriptpubkey.has_value() && !scriptpubkey->empty(), "custody.xsat::addcustody: scriptpubkey cannot be empty");

    auto scriptpubkey_idx = _custody.get_index<"scriptpubkey"_n>();
    const checksum256 hash = xsat::utils::hash(*scriptpubkey);
    auto scriptpubkey_itr = scriptpubkey_idx.find(hash);
    check(scriptpubkey_itr == scriptpubkey_idx.end(), "custody.xsat::addcustody: bitcoin address already exists");

    _custody.emplace(get_self(), [&](auto& row) {
        row.id = next_custody_id();
        row.staker = staker;
        row.proxy = proxy;
        row.validator = validator;
        row.value = 0;
        row.latest_stake_time = eosio::current_time_point();
        if (btc_address.has_value()) {
            row.btc_address = *btc_address;
            row.scriptpubkey = *scriptpubkey;
        } else {
            row.scriptpubkey = *scriptpubkey;
        }
    });
}

[[eosio::action]]
void custody::delcustody(const checksum160 staker) {
    require_auth(get_self());
    auto staker_idx = _custody.get_index<"bystaker"_n>();
    auto staker_id = xsat::utils::compute_id(staker);
    auto itr = staker_idx.require_find(staker_id, "custody.xsat::delcustody: staker does not exists");
    handle_staking(itr, 0);
    staker_idx.erase(itr);
}


[[eosio::action]]
void custody::creditstake(const checksum160& staker, const uint64_t balance) {
    require_auth(get_self());
    auto staker_id = xsat::utils::compute_id(staker);
    auto custody_staker_idx = _custody.get_index<"bystaker"_n>();
    auto custody_staker_itr = custody_staker_idx.require_find(staker_id, "custody.xsat::offchainsync: staker does not exists");
    handle_staking(custody_staker_itr, balance);
}

template <typename T>
uint64_t custody::get_current_staking_value(T& itr) {
    endorse_manage::evm_staker_table _staking(ENDORSER_MANAGE_CONTRACT, ENDORSER_MANAGE_CONTRACT.value);
    checksum256 staking_id = endorse_manage::compute_staking_id(itr->proxy, itr->staker, itr->validator);
    auto staking_idx = _staking.get_index<"bystakingid"_n>();
    auto staking_itr = staking_idx.find(staking_id);
    return staking_itr != staking_idx.end() ? staking_itr->quantity.amount : 0;
}

template <typename T>
bool custody::handle_staking(T& itr, uint64_t balance) {
    uint64_t current_staking_value = get_current_staking_value(itr);
    uint64_t new_staking_value = balance >= MAX_STAKING ? MAX_STAKING : 0;
    check(new_staking_value != current_staking_value, "custody.xsat::handle_staking: no change in staking value");

    // credit stake
    endorse_manage::creditstake_action creditstake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
    creditstake.send(get_self(), itr->proxy, itr->staker, itr->validator, asset(new_staking_value, BTC_SYMBOL));
    auto custody_staker_itr = _custody.require_find(itr->id, "custody.xsat::handle_staking: staker does not exists");
    _custody.modify(custody_staker_itr, same_payer, [&](auto& row) {
        row.value = new_staking_value;
        row.latest_stake_time = eosio::current_time_point();
    });
}

uint64_t custody::next_custody_id() {
    global_row global = _global.get_or_default();
    global.custody_id++;
    _global.set(global, get_self());
    return global.custody_id;
}
