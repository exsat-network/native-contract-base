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
    check(btc_address.has_value() || scriptpubkey.has_value(), "custody.xsat::addcustody: btc_address and scriptpubkey cannot be empty at the same time");
    custody_index _custody(get_self(), get_self().value);
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

    // stake
    utxo_manage::utxo_table _utxo(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    auto utxo_scriptpubkey_idx = _utxo.get_index<"scriptpubkey"_n>();
    auto lower = utxo_scriptpubkey_idx.lower_bound(hash);
    auto upper = utxo_scriptpubkey_idx.upper_bound(hash);
    uint64_t utxo_value = 0;
    for (auto s_itr = lower; s_itr != upper; s_itr++) {
        utxo_value += s_itr->value;
    }
    if (utxo_value > 0) {
        endorse_manage::evm_staker_action stake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
        stake.send(get_self(), proxy, staker, validator, asset(utxo_value, BTC_SYMBOL));
    }

    _custody.emplace(get_self(), [&](auto& row) {
        row.id = next_custody_id();
        row.staker = staker;
        row.proxy = proxy;
        row.validator = validator;
        row.value = utxo_value;
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
void custody::updatecusty(const checksum160 staker, const name validator) {
    require_auth(get_self());
    check(is_account(validator), "custody.xsat::updatecusty: validator does not exists");
    custody_index _custody(get_self(), get_self().value);
    auto staker_idx = _custody.get_index<"bystaker"_n>();
    auto staker_id = xsat::utils::compute_id(staker);
    auto itr = staker_idx.require_find(staker_id, "custody.xsat::updatecusty: staker does not exists");

    // newstake
    endorse_manage::evm_staker_table _staking(ENDORSER_MANAGE_CONTRACT, ENDORSER_MANAGE_CONTRACT.value);
    checksum256 staking_id = endorse_manage::compute_staking_id(itr->proxy, itr->staker, itr->validator);
    auto staking_idx = _staking.get_index<"bystakingid"_n>();
    auto staking_itr = staking_idx.find(staking_id);
    if (staking_itr != staking_idx.end() && staking_itr->quantity.amount > 0) {
        endorse_manage::evm_newstake_action stake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
        stake.send(get_self(), itr->proxy, itr->staker, itr->validator, validator, staking_itr->quantity);
    }

    staker_idx.modify(itr, same_payer, [&](auto& row) {
        row.validator = validator;
        row.latest_stake_time = eosio::current_time_point();
        });
}

[[eosio::action]]
void custody::delcustody(const checksum160 staker) {
    require_auth(get_self());
    custody_index _custody(get_self(), get_self().value);
    auto staker_idx = _custody.get_index<"bystaker"_n>();
    auto staker_id = xsat::utils::compute_id(staker);
    auto itr = staker_idx.require_find(staker_id, "custody.xsat::delcustody: staker does not exists");

    // unstake
    endorse_manage::evm_staker_table _staking(ENDORSER_MANAGE_CONTRACT, ENDORSER_MANAGE_CONTRACT.value);
    checksum256 staking_id = endorse_manage::compute_staking_id(itr->proxy, itr->staker, itr->validator);
    auto staking_idx = _staking.get_index<"bystakingid"_n>();
    auto staking_itr = staking_idx.find(staking_id);
    if (staking_itr != staking_idx.end() && staking_itr->quantity.amount > 0) {
        endorse_manage::evm_unstake_action stake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
        stake.send(get_self(), itr->proxy, itr->staker, itr->validator, staking_itr->quantity);
    }

    staker_idx.erase(itr);
}

[[eosio::action]]
void custody::syncstake(optional<uint64_t> process_rows) {
    process_rows = process_rows.value_or(100);
    global_id_row global_id = _global_id.get_or_default();
    const uint64_t height = current_irreversible_height();
    check(height > global_id.last_height, "custody.xsat::syncstake: no new block height update");

    custody_index _custody(get_self(), get_self().value);
    utxo_manage::utxo_table _utxo(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    endorse_manage::evm_staker_table _staking(ENDORSER_MANAGE_CONTRACT, ENDORSER_MANAGE_CONTRACT.value);
    auto scriptpubkey_idx = _utxo.get_index<"scriptpubkey"_n>();

    uint64_t last_custody_id = global_id.last_custody_id;
    auto itr = _custody.lower_bound(last_custody_id + 1);
    auto end = _custody.end();
    uint64_t rows = 0;
    bool reached_end = false;

    for (; itr != end && rows < process_rows; ++itr, ++rows) {
        const checksum256 hash = xsat::utils::hash(itr->scriptpubkey);
        auto lower = scriptpubkey_idx.lower_bound(hash);
        auto upper = scriptpubkey_idx.upper_bound(hash);
        uint64_t utxo_value = 0;
        for (auto s_itr = lower; s_itr != upper; s_itr++) {
            utxo_value += s_itr->value;
        }
        checksum256 staking_id = endorse_manage::compute_staking_id(itr->proxy, itr->staker, itr->validator);
        auto staking_idx = _staking.get_index<"bystakingid"_n>();
        auto staking_itr = staking_idx.find(staking_id);
        uint64_t staking_value = 0;
        if (staking_itr != staking_idx.end()) {
            staking_value = staking_itr->quantity.amount;
        }
        if (utxo_value > staking_value) {
            endorse_manage::evm_staker_action stake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
            stake.send(get_self(), itr->proxy, itr->staker, itr->validator, asset(safemath::sub(utxo_value, staking_value), BTC_SYMBOL));
            _custody.modify(itr, same_payer, [&](auto& row) {
                row.value = utxo_value;
                row.latest_stake_time = eosio::current_time_point();
            });
        } else if (utxo_value < staking_value) {
            endorse_manage::evm_unstake_action stake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
            stake.send(get_self(), itr->proxy, itr->staker, itr->validator, asset(safemath::sub(staking_value, utxo_value), BTC_SYMBOL));
            _custody.modify(itr, same_payer, [&](auto& row) {
                row.value = utxo_value;
                row.latest_stake_time = eosio::current_time_point();
            });
        }
        last_custody_id = itr->id;
        if (std::next(itr) == end) {
            reached_end = true;
            last_custody_id = 0;
            break;
        }
    }
    if (reached_end || rows == process_rows) {
        global_id.last_custody_id = last_custody_id;
        if (reached_end) {
            global_id.last_height = height;
        }
        _global_id.set(global_id, get_self());
    }
}

uint64_t custody::next_custody_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.custody_id++;
    _global_id.set(global_id, get_self());
    return global_id.custody_id;
}

uint64_t custody::current_irreversible_height() {
    utxo_manage::chain_state_table _chain_state(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    auto chain_state = _chain_state.get_or_default();
    return chain_state.irreversible_height;
}
