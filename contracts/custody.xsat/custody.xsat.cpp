#include <custody.xsat/custody.xsat.hpp>

#ifdef DEBUG
#include <bitcoin/script/address.hpp>
#include "./src/debug.hpp"
#endif

[[eosio::action]]
void custody::addcustody(const checksum160 staker, const checksum160 proxy, const name validator, bool is_issue, const optional<string> btc_address, optional<vector<uint8_t>> scriptpubkey) {
    require_auth(get_self());
    check(is_account(validator), "custody.xsat::addcustody: validator does not exists");
    check(proxy != checksum160(), "custody.xsat::addcustody: proxy cannot be empty");
    check(btc_address.has_value() || scriptpubkey.has_value(), "custody.xsat::addcustody: btc_address and scriptpubkey cannot be empty at the same time");
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

    uint64_t utxo_value = 0;
    global_row global = _global.get_or_default();
    if (global.is_synchronized) {
        // stake
        utxo_manage::utxo_table _utxo(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
        auto utxo_scriptpubkey_idx = _utxo.get_index<"scriptpubkey"_n>();
        auto lower = utxo_scriptpubkey_idx.lower_bound(hash);
        auto upper = utxo_scriptpubkey_idx.upper_bound(hash);
        for (auto s_itr = lower; s_itr != upper; s_itr++) {
            utxo_value += s_itr->value;
        }
        if (utxo_value > 0) {
            endorse_manage::evm_stake_action stake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
            stake.send(get_self(), proxy, staker, validator, asset(utxo_value, BTC_SYMBOL));
        }
    }

    _custody.emplace(get_self(), [&](auto& row) {
        row.id = next_custody_id();
        row.staker = staker;
        row.proxy = proxy;
        row.validator = validator;
        row.value = utxo_value;
        row.is_issue = is_issue;
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
void custody::updcustody(const checksum160 staker, const name validator) {
    require_auth(get_self());
    check(is_account(validator), "custody.xsat::updcustody: validator does not exists");
    auto staker_idx = _custody.get_index<"bystaker"_n>();
    auto staker_id = xsat::utils::compute_id(staker);
    auto itr = staker_idx.require_find(staker_id, "custody.xsat::updcustody: staker does not exists");

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
void custody::updblkstatus(const bool is_synchronized) {
    require_auth(get_self());
    global_row global = _global.get_or_default();
    global.is_synchronized = is_synchronized;
    _global.set(global, get_self());
}

[[eosio::action]]
void custody::syncstake(optional<uint64_t> process_rows) {
    global_row global = _global.get_or_default();
    check(global.is_synchronized, "custody.xsat::syncstake: bitcoin block is not synchronized");

    process_rows = process_rows.value_or(100);
    const uint64_t height = current_irreversible_height();
    check(height > global.last_height, "custody.xsat::syncstake: no new block height update");

    utxo_manage::utxo_table _utxo(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    endorse_manage::evm_staker_table _staking(ENDORSER_MANAGE_CONTRACT, ENDORSER_MANAGE_CONTRACT.value);
    auto scriptpubkey_idx = _utxo.get_index<"scriptpubkey"_n>();

    uint64_t last_custody_id = global.last_custody_id;
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
            endorse_manage::evm_stake_action stake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
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
        global.last_custody_id = last_custody_id;
        if (reached_end) {
            global.last_height = height;
        }
        _global.set(global, get_self());
    }
}

[[eosio::action]]
void custody::stake(const checksum160& staker, const asset& quantity) {
    require_auth(get_self());
    global_row global = _global.get_or_default();
    check(global.is_synchronized == false, "custody.xsat::stake: bitcoin block is synchronized, this stake channel is closed");
    check(quantity.amount > 0, "custody.xsat::stake: quantity must be positive");
    auto staker_id = xsat::utils::compute_id(staker);
    auto custody_staker_idx = _custody.get_index<"bystaker"_n>();
    auto custody_staker_itr = custody_staker_idx.require_find(staker_id, "custody.xsat::stake: staker does not exists");

    //evm stake
    endorse_manage::evm_stake_action stake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
    stake.send(get_self(), custody_staker_itr->proxy, staker, custody_staker_itr->validator, quantity);

    if (custody_staker_itr->is_issue) {
        auto vault_staker_idx = _vault.get_index<"bystaker"_n>();
        auto vault_staker_itr = vault_staker_idx.find(staker_id);

        // issue BTC
        btc::issue_action issue(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
        issue.send(BTC_CONTRACT, quantity, "issue BTC to staker");
        // transfer BTC to vault
        btc::transfer_action transfer(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
        transfer.send(BTC_CONTRACT, VAULT_ACCOUNT, quantity, "transfer BTC to vault");

        if (vault_staker_itr == vault_staker_idx.end()) {
            _vault.emplace(get_self(), [&](auto& row) {
                row.id = next_vault_id();
                row.staker = staker;
                row.validator = custody_staker_itr->validator;
                row.btc_address = custody_staker_itr->btc_address;
                row.quantity = quantity;
            });
        } else {
            vault_staker_idx.modify(vault_staker_itr, same_payer, [&](auto& row) {
                row.quantity += quantity;
            });
        }
    }
}

[[eosio::action]]
void custody::unstake(const checksum160& staker, const asset& quantity) {
    require_auth(get_self());
    global_row global = _global.get_or_default();
    check(global.is_synchronized == false, "custody.xsat::stake: bitcoin block is synchronized, this stake channel is closed");
    check(quantity.amount > 0, "custody.xsat::unstake: quantity must be positive");
    auto staker_id = xsat::utils::compute_id(staker);
    auto custody_staker_idx = _custody.get_index<"bystaker"_n>();
    auto custody_staker_itr = custody_staker_idx.require_find(staker_id, "custody.xsat::unstake: staker does not exists in custody");

    //evm unstake
    endorse_manage::evm_unstake_action unstake(ENDORSER_MANAGE_CONTRACT, { get_self(), "active"_n });
    unstake.send(get_self(), custody_staker_itr->proxy, staker, custody_staker_itr->validator, quantity);

    if (custody_staker_itr->is_issue) {
        auto vault_staker_idx = _vault.get_index<"bystaker"_n>();
        auto vault_staker_itr = vault_staker_idx.require_find(staker_id, "custody.xsat::unstake: staker does not exists in vault");
        check(vault_staker_itr->quantity >= quantity, "custody.xsat::unstake: insufficient quantity in vault");

        // transfer BTC from vault to custody
        btc::transfer_action transfer(BTC_CONTRACT, { VAULT_ACCOUNT, "active"_n });
        transfer.send(VAULT_ACCOUNT, BTC_CONTRACT, quantity, "transfer BTC from vault to custody");
        // retire BTC
        btc::retire_action retire(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
        retire.send(quantity, "retire BTC from staker");

        vault_staker_idx.modify(vault_staker_itr, same_payer, [&](auto& row) {
            row.quantity -= quantity;
        });
    }

}

uint64_t custody::next_custody_id() {
    global_row global = _global.get_or_default();
    global.custody_id++;
    _global.set(global, get_self());
    return global.custody_id;
}

uint64_t custody::next_vault_id() {
    global_row global = _global.get_or_default();
    global.vault_id++;
    _global.set(global, get_self());
    return global.vault_id;
}

uint64_t custody::current_irreversible_height() {
    utxo_manage::chain_state_table _chain_state(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    auto chain_state = _chain_state.get_or_default();
    return chain_state.irreversible_height;
}
