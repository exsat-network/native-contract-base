#include <brdgmng.xsat/brdgmng.xsat.hpp>

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

[[eosio::action]]
void brdgmng::initialize() {
    require_auth(BOOT_ACCOUNT);
    boot_row boot = _boot.get_or_default();
    check(!boot.initialized, "brdgmng.xsat::boot: already initialized");
    // initialize issue 1 BTC
    btc::issue_action issue(BTC_CONTRACT, {BTC_CONTRACT, "active"_n});
    asset quantity = asset(BTC_BASE, BTC_SYMBOL);
    issue.send(BTC_CONTRACT, quantity, "initialize issue one BTC to boot.xsat");
    // transfer BTC to boot.xsat
    btc::transfer_action transfer(BTC_CONTRACT, {BTC_CONTRACT, "active"_n});
    transfer.send(BTC_CONTRACT, BOOT_ACCOUNT, quantity, "transfer one BTC to boot.xsat");
    boot.initialized = true;
    boot.returned = false;
    boot.quantity = quantity;
    _boot.set(boot, get_self());
}

[[eosio::action]]
void brdgmng::updateconfig(bool deposit_enable, bool withdraw_enable, bool check_uxto_enable, uint64_t limit_amount, uint64_t deposit_fee,
                           uint64_t withdraw_fast_fee, uint64_t withdraw_slow_fee, uint16_t withdraw_merge_count, uint16_t withdraw_timeout_minutes,
                           uint16_t btc_address_inactive_clear_days) {
    require_auth(get_self());
    config_row config = _config.get_or_default();
    config.deposit_enable = deposit_enable;
    config.withdraw_enable = withdraw_enable;
    config.check_uxto_enable = check_uxto_enable;
    config.limit_amount = limit_amount;
    config.deposit_fee = deposit_fee;
    config.withdraw_fast_fee = withdraw_fast_fee;
    config.withdraw_slow_fee = withdraw_slow_fee;
    config.withdraw_merge_count = withdraw_merge_count;
    config.withdraw_timeout_minutes = withdraw_timeout_minutes;
    config.btc_address_inactive_clear_days = btc_address_inactive_clear_days;
    _config.set(config, get_self());
}

[[eosio::action]]
void brdgmng::addperm(const uint64_t id, const vector<name>& actors) {
    require_auth(get_self());
    if (actors.empty()) {
        check(false, "brdgmng.xsat::addperm: actors cannot be empty");
    }
    for (const auto& actor : actors) {
        check(is_account(actor), "brdgmng.xsat::addperm: actor " + actor.to_string() + " does not exist");
    }
    auto itr = _permission.find(id);
    check(itr == _permission.end(), "brdgmng.xsat::addperm: permission id already exists");
    _permission.emplace(get_self(), [&](auto& row) {
        row.id = id;
        row.actors = actors;
    });
}

[[eosio::action]]
void brdgmng::delperm(const uint64_t id) {
    require_auth(get_self());
    auto itr = _permission.require_find(id, "brdgmng.xsat::delperm: permission id does not exists");
    _permission.erase(itr);
}

[[eosio::action]]
void brdgmng::addaddresses(const name& actor, const uint64_t permission_id, string b_id, string wallet_code, const vector<string>& btc_addresses) {
    check_first_actor_permission(actor, permission_id);
    if (btc_addresses.empty()) {
        check(false, "brdgmng.xsat::addaddresses: btc_addresses cannot be empty");
    }
    address_index _address = address_index(_self, permission_id);
    address_mapping_index _address_mapping = address_mapping_index(_self, permission_id);
    auto address_idx = _address.get_index<"bybtcaddr"_n>();
    auto address_mapping_idx = _address_mapping.get_index<"bybtcaddr"_n>();
    for (const auto& btc_address : btc_addresses) {
        auto scriptpubkey = bitcoin::utility::address_to_scriptpubkey(btc_address);

        checksum256 btc_addrress_hash = xsat::utils::hash(btc_address);
        auto address_itr = address_idx.find(btc_addrress_hash);
        check(address_itr == address_idx.end(), "brdgmng.xsat::addaddresses: btc_address already exists in address");
        auto address_mapping_itr = address_mapping_idx.find(btc_addrress_hash);
        check(address_mapping_itr == address_mapping_idx.end(), "brdgmng.xsat::addaddresses: btc_address already exists in address_mapping");

        _address.emplace(get_self(), [&](auto& row) {
            row.id = next_address_id();
            row.permission_id = permission_id;
            row.b_id = b_id;
            row.wallet_code = wallet_code;
            row.provider_actors = vector<name>{actor};
            row.statuses = vector<address_status>{address_status_initiated};
            row.confirmed_count = 1;
            row.global_status = global_status_initiated;
            row.btc_address = btc_address;
        });
    }
    statistics_table _statistics = statistics_table(_self, permission_id);
    statistics_row statistics = _statistics.get_or_default();
    statistics.total_btc_address_count += btc_addresses.size();
    _statistics.set(statistics, get_self());
}

[[eosio::action]]
void brdgmng::valaddress(const name& actor, const uint64_t permission_id, const uint64_t address_id, const address_status status) {
    check_permission(actor, permission_id);

    address_index _address = address_index(_self, permission_id);
    address_mapping_index _address_mapping = address_mapping_index(_self, permission_id);
    auto address_itr = _address.require_find(address_id, "brdgmng.xsat::valaddress: address_id does not exists in address");
    check(address_itr->global_status != global_status_succeed, "brdgmng.xsat::valaddress: btc_address status is already confirmed");
    check(is_final_address_status(status), "brdgmng.xsat::valaddress: invalid status");

    auto actor_itr = std::find_if(address_itr->provider_actors.begin(), address_itr->provider_actors.end(), [&](const auto& u) { return u == actor; });
    check(actor_itr == address_itr->provider_actors.end(), "brdgmng.xsat::valaddress: actor already exists in the provider actors list");
    _address.modify(address_itr, same_payer, [&](auto& row) {
        row.provider_actors.push_back(actor);
        row.statuses.push_back(status);
        if (status == address_status_confirmed) {
            row.confirmed_count++;
            if (verify_providers(_permission.get(permission_id).actors, address_itr->provider_actors) &&
                row.confirmed_count >= address_itr->provider_actors.size()) {
                row.global_status = global_status_succeed;
            }
        } else {
            row.global_status = global_status_failed;
        }
    });
}

[[eosio::action]]
void brdgmng::mappingaddr(const name& actor, const uint64_t permission_id, const checksum160& evm_address) {
    check_permission(actor, permission_id);

    address_index _address = address_index(_self, permission_id);
    address_mapping_index _address_mapping = address_mapping_index(_self, permission_id);
    auto evm_address_mapping_idx = _address_mapping.get_index<"byevmaddr"_n>();
    checksum256 evm_address_hash = xsat::utils::compute_id(evm_address);
    auto evm_address_mapping_itr = evm_address_mapping_idx.find(evm_address_hash);
    check(evm_address_mapping_itr == evm_address_mapping_idx.end(), "brdgmng.xsat::mappingaddr: evm_address already exists in address_mapping");

    string btc_address;
    auto address_itr = _address.begin();
    for (; address_itr != _address.end(); ++address_itr) {
        if (address_itr->global_status == global_status_succeed) {
            btc_address = address_itr->btc_address;
            break;
        }
    }
    check(!btc_address.empty(), "brdgmng.xsat::mappingaddr: the confirmed btc address with permission id does not exists in address");

    auto btc_address_mapping_idx = _address_mapping.get_index<"bybtcaddr"_n>();
    checksum256 btc_address_hash = xsat::utils::hash(btc_address);
    auto btc_address_mapping_itr = btc_address_mapping_idx.find(btc_address_hash);
    check(btc_address_mapping_itr == btc_address_mapping_idx.end(), "brdgmng.xsat::mappingaddr: btc_address already exists in address_mapping");

    _address_mapping.emplace(get_self(), [&](auto& row) {
        row.id = next_address_mapping_id();
        row.b_id = address_itr->b_id;
        row.wallet_code = address_itr->wallet_code;
        row.btc_address = btc_address;
        row.evm_address = evm_address;
    });

    auto address_id_itr = _address.require_find(address_itr->id, "brdgmng.xsat::mappingaddr: address id does not exists in address");
    _address.erase(address_id_itr);

    statistics_table _statistics = statistics_table(_self, permission_id);
    statistics_row statistics = _statistics.get_or_default();
    statistics.mapped_address_count++;
    _statistics.set(statistics, get_self());

    // log
    brdgmng::mapaddrlog_action _mapaddrlog(get_self(), {get_self(), "active"_n});
    _mapaddrlog.send(actor, permission_id, address_itr->id, evm_address, btc_address);
}

[[eosio::action]]
void brdgmng::deposit(const name& actor, const uint64_t permission_id, const string& b_id, const string& wallet_code, const string& btc_address,
                      const string& order_id, const uint64_t block_height, const checksum256& tx_id, const uint32_t index, const uint64_t amount,
                      const tx_status tx_status, const optional<string>& remark_detail, const uint64_t tx_time_stamp, const uint64_t create_time_stamp) {
    check_first_actor_permission(actor, permission_id);
    config_row config = _config.get_or_default();
    check(config.deposit_enable, "brdgmng.xsat::deposit: deposit is disabled");
    check(amount > 0, "brdgmng.xsat::deposit: amount must be positive");

    // check order_id is unique in the deposit table
    deposit_index _deposit_confirmed = deposit_index(_self, permission_id);
    auto deposit_idx_confirmed = _deposit_confirmed.get_index<"byorderid"_n>();
    auto deposit_itr_confirmed = deposit_idx_confirmed.find(xsat::utils::hash(order_id));
    check(deposit_itr_confirmed == deposit_idx_confirmed.end(), "6002:brdgmng.xsat::deposit: order_id already exists in deposit");

    check(!btc_address.empty(), "brdgmng.xsat::deposit: btc_address cannot be empty, must be a valid BTC address");
    address_mapping_index _address_mapping = address_mapping_index(_self, permission_id);
    auto btc_address_mapping_idx = _address_mapping.get_index<"bybtcaddr"_n>();
    checksum256 btc_address_hash = xsat::utils::hash(btc_address);
    auto btc_address_mapping_itr = btc_address_mapping_idx.require_find(btc_address_hash, "brdgmng.xsat::deposit: bitcoin address does not map evm address yet");

    depositing_index _deposit_pending = depositing_index(_self, permission_id);
    auto deposit_idx_pending = _deposit_pending.get_index<"byorderid"_n>();
    auto deposit_itr_pending = deposit_idx_pending.find(xsat::utils::hash(order_id));
    // check(deposit_itr_pending == deposit_idx_pending.end(), "6001:brdgmng.xsat::deposit: order_id already exists in deposit");
    if (deposit_itr_pending != deposit_idx_pending.end()) {
        deposit_idx_pending.modify(deposit_itr_pending, get_self(), [&](auto& row) {
            row.b_id = b_id;
            row.wallet_code = wallet_code;
            row.btc_address = btc_address;
            row.evm_address = btc_address_mapping_itr->evm_address;
            row.block_height = block_height;
            row.tx_id = tx_id;
            row.index = index;
            row.amount = amount;
            row.fee = config.deposit_fee;
            row.tx_time_stamp = tx_time_stamp;
            row.create_time_stamp = create_time_stamp;
            if (!is_final_tx_status(deposit_itr_pending->tx_status)) {
                row.tx_status = tx_status;
            }
            if (remark_detail.has_value()) {
                row.remark_detail = *remark_detail;
            }
        });
    } else {
        _deposit_pending.emplace(get_self(), [&](auto& row) {
            row.id = next_deposit_id();
            row.permission_id = permission_id;
            row.global_status = global_status_initiated;
            row.b_id = b_id;
            row.wallet_code = wallet_code;
            row.btc_address = btc_address;
            row.evm_address = btc_address_mapping_itr->evm_address;
            row.order_id = order_id;
            row.block_height = block_height;
            row.tx_id = tx_id;
            row.index = index;
            row.amount = amount;
            row.fee = config.deposit_fee;
            row.tx_status = tx_status;
            row.tx_time_stamp = tx_time_stamp;
            row.create_time_stamp = create_time_stamp;
            if (remark_detail.has_value()) {
                row.remark_detail = *remark_detail;
            }
        });
    }
}

[[eosio::action]]
void brdgmng::valdeposit(const name& actor, const uint64_t permission_id, const uint64_t deposit_id, const tx_status tx_status) {
    check_permission(actor, permission_id);
    depositing_index _deposit_pending = depositing_index(_self, permission_id);
    deposit_index _deposit_confirmed = deposit_index(_self, permission_id);
    auto deposit_itr_confirmed = _deposit_confirmed.find(deposit_id);
    if (deposit_itr_confirmed != _deposit_confirmed.end()) {
        check(deposit_itr_confirmed->global_status != global_status_succeed, "6003:brdgmng.xsat::valdeposit: deposit status is already succeed");
        check(deposit_itr_confirmed->global_status != global_status_failed, "6004:brdgmng.xsat::valdeposit: deposit status is already failed");
    }
    auto deposit_itr_pending = _deposit_pending.require_find(deposit_id, "6005:brdgmng.xsat::valdeposit: deposit id does not exists");
    check(is_final_tx_status(deposit_itr_pending->tx_status) && is_final_tx_status(tx_status), "brdgmng.xsat::valdeposit: only final tx status can be confirmed");
    auto actor_itr = std::find_if(deposit_itr_pending->provider_actors.begin(), deposit_itr_pending->provider_actors.end(), [&](const auto& u) { return u == actor; });
    check(actor_itr == deposit_itr_pending->provider_actors.end(), "6010:brdgmng.xsat::valdeposit: actor already exists in the provider actors list");
    // // The creator can update the tx_status until the final state, while non-creators can only wait until the final state and then confirm
    // name creator = deposit_itr_pending->provider_actors[0];
    // if (actor != creator) {
    //     check(is_final_tx_status(tx_status), "brdgmng.xsat::valdeposit: only creator can modify the tx_status to confirmed, failed or rollbacked");
    //     check(is_final_tx_status(deposit_itr_pending->tx_status), "brdgmng.xsat::valdeposit: only final tx_status can be confirmed by non-creator");
    // }
    _deposit_pending.modify(deposit_itr_pending, same_payer, [&](auto& row) {
        row.tx_status = tx_status;
        row.provider_actors.push_back(actor);
        if (tx_status == tx_status_confirmed) {
            row.confirmed_count++;
            if (verify_providers(_permission.get(permission_id).actors, row.provider_actors) && row.confirmed_count >= row.provider_actors.size()) {
                row.global_status = global_status_succeed;
            }
        } else if (tx_status == tx_status_failed || tx_status == tx_status_rollbacked) {
            row.global_status = global_status_failed;
        }
    });
    // move pending deposit to confirmed deposit
    config_row config = _config.get_or_default();
    if (deposit_itr_pending->global_status == global_status_succeed || deposit_itr_pending->global_status == global_status_failed) {
        if (deposit_itr_pending->global_status == global_status_succeed && deposit_itr_pending->amount >= config.limit_amount) {
            if (config.check_uxto_enable) {
                bool uxto_exist = check_utxo_exist(deposit_itr_pending->tx_id, deposit_itr_pending->index);
                check(uxto_exist, "brdgmng.xsat::valdeposit: utxo does not exist");
            }
            const uint64_t issue_amount = safemath::sub(deposit_itr_pending->amount, deposit_itr_pending->fee);
            handle_btc_deposit(permission_id, issue_amount, deposit_itr_pending->evm_address);
        }
        _deposit_confirmed.emplace(get_self(), [&](auto& row) {
            row.id = deposit_itr_pending->id;
            row.permission_id = deposit_itr_pending->permission_id;
            row.provider_actors = deposit_itr_pending->provider_actors;
            row.confirmed_count = deposit_itr_pending->confirmed_count;
            row.tx_status = deposit_itr_pending->tx_status;
            row.global_status = deposit_itr_pending->global_status;
            row.b_id = deposit_itr_pending->b_id;
            row.wallet_code = deposit_itr_pending->wallet_code;
            row.btc_address = deposit_itr_pending->btc_address;
            row.evm_address = deposit_itr_pending->evm_address;
            row.order_id = deposit_itr_pending->order_id;
            row.block_height = deposit_itr_pending->block_height;
            row.tx_id = deposit_itr_pending->tx_id;
            row.index = deposit_itr_pending->index;
            row.amount = deposit_itr_pending->amount;
            row.fee = deposit_itr_pending->fee;
            row.remark_detail = deposit_itr_pending->remark_detail;
            row.tx_time_stamp = deposit_itr_pending->tx_time_stamp;
            row.create_time_stamp = deposit_itr_pending->create_time_stamp;
        });

        // log
        brdgmng::depositlog_action _depositlog(get_self(), {get_self(), "active"_n});
        _depositlog.send(deposit_itr_pending->permission_id, deposit_itr_pending->id, deposit_itr_pending->b_id, deposit_itr_pending->wallet_code,
                         deposit_itr_pending->global_status, deposit_itr_pending->btc_address, deposit_itr_pending->evm_address, deposit_itr_pending->order_id,
                         deposit_itr_pending->block_height, deposit_itr_pending->tx_id, deposit_itr_pending->index, deposit_itr_pending->amount,
                         deposit_itr_pending->fee, deposit_itr_pending->remark_detail, deposit_itr_pending->tx_time_stamp,
                         deposit_itr_pending->create_time_stamp);

        _deposit_pending.erase(deposit_itr_pending);
    }
}

[[eosio::on_notify("*::transfer")]]
void brdgmng::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    // ignore transfers
    if (to != get_self()) return;

    const name contract = get_first_receiver();
    check(contract == BTC_CONTRACT && quantity.symbol == BTC_SYMBOL, "brdgmng.xsat: only transfer [btc.xsat/BTC]");
    if (from == BOOT_ACCOUNT) {
        do_return(from, contract, quantity, memo);
    } else {
        do_withdraw(from, contract, quantity, memo);
    }
}

void brdgmng::do_return(const name& from, const name& contract, const asset& quantity, const string& memo) {
    check(from == BOOT_ACCOUNT, "brdgmng.xsat: only return from [boot.xsat]");
    boot_row boot = _boot.get_or_default();
    check(!boot.returned, "brdgmng.xsat: already returned");
    check(quantity == boot.quantity, "brdgmng.xsat: refund amount does not match, must be " + boot.quantity.to_string());
    // burn BTC
    btc::transfer_action transfer(contract, {get_self(), "active"_n});
    transfer.send(get_self(), BTC_CONTRACT, quantity, "return BTC to btc.xsat");
    btc::retire_action retire(contract, {BTC_CONTRACT, "active"_n});
    retire.send(quantity, "retire BTC from boot.xsat");

    boot.returned = true;
    _boot.set(boot, get_self());
}

void brdgmng::do_withdraw(const name& from, const name& contract, const asset& quantity, const string& memo) {
    config_row config = _config.get_or_default();
    check(config.withdraw_enable, "brdgmng.xsat: withdraw is disabled");
    check(quantity.amount > 0, "brdgmng.xsat: must transfer positive quantity");
    check(quantity.amount >= config.limit_amount, "brdgmng.xsat: withdraw amount must be more than " + quantity.to_string());
    auto parts = xsat::utils::split(memo, ",");
    check(parts.size() == 4, "brdgmng.xsat: invalid memo ex: \"<permission_id>,<evm_address>,<btc_address>,<gas_level>\"");
    uint64_t permission_id = std::stoull(parts[0]);
    auto permission_itr = _permission.require_find(permission_id, "brdgmng.xsat: permission id does not exists");
    withdrawing_index _withdraw_pending = withdrawing_index(_self, permission_id);
    uint64_t withdraw_id = next_withdraw_id();
    _withdraw_pending.emplace(get_self(), [&](auto& row) {
        row.id = withdraw_id;
        row.permission_id = permission_id;
        row.evm_address = xsat::utils::evm_address_to_checksum160(parts[1]);
        row.btc_address = parts[2];
        row.gas_level = parts[3];
        row.global_status = global_status_initiated;
        row.withdraw_time_stamp = current_time_point().sec_since_epoch();
        if (row.gas_level == "fast") {
            row.order_no = generate_order_no({withdraw_id});
            row.fee = config.withdraw_fast_fee;
        } else {
            row.order_no = "";
            row.fee = config.withdraw_slow_fee;
        }
        row.amount = quantity.amount - row.fee;
    });
}

[[eosio::action]]
void brdgmng::genorderno(const uint64_t permission_id) {
    auto permission_itr = _permission.require_find(permission_id, "brdgmng.xsat::genorderno: permission id does not exists");
    withdrawing_index _withdraw_pending = withdrawing_index(_self, permission_id);
    auto withdraw_idx_pending = _withdraw_pending.get_index<"byorderno"_n>();
    checksum256 empty_order_no_hash = xsat::utils::hash("");
    auto lower_bound = withdraw_idx_pending.lower_bound(empty_order_no_hash);
    auto upper_bound = withdraw_idx_pending.upper_bound(empty_order_no_hash);

    std::vector<uint64_t> pending_ids;
    uint64_t count = 0;
    uint64_t earliest_timestamp = std::numeric_limits<uint64_t>::max();

    config_row config = _config.get_or_default();
    uint64_t current_time = current_time_point().sec_since_epoch();
    check(lower_bound != upper_bound, "brdgmng.xsat::genorderno: pending withdraw order is empty");
    for (auto itr = lower_bound; itr != upper_bound; ++itr) {
        pending_ids.push_back(itr->id);
        count++;
        earliest_timestamp = std::min(earliest_timestamp, itr->withdraw_time_stamp);
        if (count >= config.withdraw_merge_count) {
            break;
        }
    }
    check((count >= config.withdraw_merge_count || (current_time - earliest_timestamp) / 60 >= config.withdraw_timeout_minutes),
          "brdgmng.xsat::genorderno: pending withdraw order is not enough or not timeout");
    string new_order_no = generate_order_no(pending_ids);
    for (const auto& id : pending_ids) {
        auto update_itr = _withdraw_pending.require_find(id, "brdgmng.xsat::genorderno: withdraw id does not exists");
        _withdraw_pending.modify(update_itr, same_payer, [&](auto& row) { row.order_no = new_order_no; });
    }
}

[[eosio::action]]
void brdgmng::withdrawinfo(const name& actor, const uint64_t permission_id, const uint64_t withdraw_id, const string& b_id, const string& wallet_code,
                           const string& order_id, const withdraw_status withdraw_status, const order_status order_status, const uint64_t block_height,
                           const checksum256& tx_id, const optional<string>& remark_detail, const uint64_t tx_time_stamp, const uint64_t create_time_stamp) {
    check_first_actor_permission(actor, permission_id);
    withdrawing_index _withdraw_pending = withdrawing_index(_self, permission_id);
    auto withdraw_itr_pending = _withdraw_pending.require_find(withdraw_id, "6011:brdgmng.xsat::withdrawinfo: withdraw id does not exists");
    check(withdraw_status >= withdraw_itr_pending->withdraw_status, "6012:brdgmng.xsat::withdrawinfo: the withdraw_status must increase forward");
    _withdraw_pending.modify(withdraw_itr_pending, same_payer, [&](auto& row) {
        row.b_id = b_id;
        row.wallet_code = wallet_code;
        row.order_id = order_id;
        row.withdraw_status = withdraw_status;
        row.block_height = block_height;
        row.tx_id = tx_id;
        row.tx_time_stamp = tx_time_stamp;
        row.create_time_stamp = create_time_stamp;
        if (!is_final_order_status(withdraw_itr_pending->order_status)) {
            row.order_status = order_status;
        }
        if (remark_detail.has_value()) {
            row.remark_detail = *remark_detail;
        }
    });
}

// [[eosio::action]]
// void brdgmng::updwithdraw(const name& actor, const uint64_t permission_id, const uint64_t withdraw_id, const withdraw_status withdraw_status,
//                           const order_status order_status, const optional<string>& remark_detail) {
//     check_first_actor_permission(actor, permission_id);
//     withdrawing_index _withdraw_pending = withdrawing_index(_self, permission_id);
//     auto withdraw_itr_pending = _withdraw_pending.require_find(withdraw_id, "brdgmng.xsat::updwithdraw: withdraw id does not exists");
//     check(withdraw_status >= withdraw_itr_pending->withdraw_status, "brdgmng.xsat::updwithdraw: the withdraw_status must increase forward");

//     _withdraw_pending.modify(withdraw_itr_pending, same_payer, [&](auto& row) {
//         row.withdraw_status = withdraw_status;
//         if (!is_final_order_status(withdraw_itr_pending->order_status)) {
//             row.order_status = order_status;
//         }
//     });
// }

[[eosio::action]]
void brdgmng::valwithdraw(const name& actor, const uint64_t permission_id, const uint64_t withdraw_id, const order_status order_status) {
    check_permission(actor, permission_id);
    withdrawing_index _withdraw_pending = withdrawing_index(_self, permission_id);
    withdraw_index _withdraw_confirmed = withdraw_index(_self, permission_id);
    auto withdraw_itr_confirmed = _withdraw_confirmed.find(withdraw_id);
    if (withdraw_itr_confirmed != _withdraw_confirmed.end()) {
        check(withdraw_itr_confirmed->global_status != global_status_succeed, "6006:brdgmng.xsat::valwithdraw: withdraw status is already succeed");
        check(withdraw_itr_confirmed->global_status != global_status_failed, "6007:brdgmng.xsat::valwithdraw: withdraw status is already failed");
    }
    auto withdraw_itr_pending = _withdraw_pending.require_find(withdraw_id, "6008:brdgmng.xsat::valwithdraw: withdraw id does not exists");
    check(is_final_order_status(withdraw_itr_pending->order_status) && is_final_order_status(order_status), "6009:brdgmng.xsat::valwithdraw: only final order status can be confirmed");

    auto actor_itr = std::find_if(withdraw_itr_pending->provider_actors.begin(), withdraw_itr_pending->provider_actors.end(), [&](const auto& u) { return u == actor; });
    check(actor_itr == withdraw_itr_pending->provider_actors.end(), "6010:brdgmng.xsat::valwithdraw: actor already exists in the provider actors list");

    // if (is_final_order_status(withdraw_itr_pending->order_status)) {
    //     check(is_final_order_status(order_status), "brdgmng.xsat::valwithdraw: the order_status is final and cannot be modified");
    //     auto actor_itr = std::find_if(withdraw_itr_pending->provider_actors.begin(), withdraw_itr_pending->provider_actors.end(), [&](const auto& u) { return u == actor; });
    //     check(actor_itr == withdraw_itr_pending->provider_actors.end(), "brdgmng.xsat::valwithdraw: actor already exists in the provider actors list");
    // }
    // name creator;
    // if (withdraw_itr_pending->provider_actors.size() >= 1) {
    //     creator = withdraw_itr_pending->provider_actors[0];
    //     if (actor != creator) {
    //         check(is_final_order_status(order_status),
    //               "brdgmng.xsat::valwithdraw: non-creators can only modify order status to partially_failed, failed, finished, canceled");
    //         check(is_final_order_status(withdraw_itr_pending->order_status),
    //               "brdgmng.xsat::valwithdraw: non-creators can only confirm the final status of the order");
    //     }
    // }

    _withdraw_pending.modify(withdraw_itr_pending, same_payer, [&](auto& row) {
        row.order_status = order_status;
        row.provider_actors.push_back(actor);
        if (order_status == order_status_finished) {
            row.confirmed_count++;
            if (verify_providers(_permission.get(permission_id).actors, row.provider_actors) && row.confirmed_count >= row.provider_actors.size()) {
                row.global_status = global_status_succeed;
            }
        } else if (order_status == order_status_failed || order_status == order_status_partially_failed || order_status == order_status_canceled) {
            row.global_status = global_status_failed;
        }
    });
    // move pending withdraw to confirmed withdraw
    if (withdraw_itr_pending->global_status == global_status_succeed || withdraw_itr_pending->global_status == global_status_failed) {
        if (withdraw_itr_pending->global_status == global_status_succeed) {
            handle_btc_withdraw(permission_id, withdraw_itr_pending->amount);
        }
        _withdraw_confirmed.emplace(get_self(), [&](auto& row) {
            row.id = withdraw_itr_pending->id;
            row.permission_id = withdraw_itr_pending->permission_id;
            row.provider_actors = withdraw_itr_pending->provider_actors;
            row.confirmed_count = withdraw_itr_pending->confirmed_count;
            row.order_status = withdraw_itr_pending->order_status;
            row.withdraw_status = withdraw_itr_pending->withdraw_status;
            row.global_status = withdraw_itr_pending->global_status;
            row.b_id = withdraw_itr_pending->b_id;
            row.wallet_code = withdraw_itr_pending->wallet_code;
            row.btc_address = withdraw_itr_pending->btc_address;
            row.evm_address = withdraw_itr_pending->evm_address;
            row.gas_level = withdraw_itr_pending->gas_level;
            row.order_id = withdraw_itr_pending->order_id;
            row.order_no = withdraw_itr_pending->order_no;
            row.block_height = withdraw_itr_pending->block_height;
            row.tx_id = withdraw_itr_pending->tx_id;
            row.amount = withdraw_itr_pending->amount;
            row.fee = withdraw_itr_pending->fee;
            row.remark_detail = withdraw_itr_pending->remark_detail;
            row.tx_time_stamp = withdraw_itr_pending->tx_time_stamp;
            row.create_time_stamp = withdraw_itr_pending->create_time_stamp;
        });

        // log
        brdgmng::withdrawlog_action _withdrawlog(get_self(), {get_self(), "active"_n});
        _withdrawlog.send(withdraw_itr_pending->permission_id, withdraw_itr_pending->id, withdraw_itr_pending->b_id, withdraw_itr_pending->wallet_code,
                          withdraw_itr_pending->global_status, withdraw_itr_pending->btc_address, withdraw_itr_pending->evm_address,
                          withdraw_itr_pending->order_id, withdraw_itr_pending->order_no, withdraw_itr_pending->withdraw_status,
                          withdraw_itr_pending->order_status, withdraw_itr_pending->block_height, withdraw_itr_pending->tx_id, withdraw_itr_pending->amount,
                          withdraw_itr_pending->fee, withdraw_itr_pending->remark_detail, withdraw_itr_pending->tx_time_stamp,
                          withdraw_itr_pending->create_time_stamp);

        _withdraw_pending.erase(withdraw_itr_pending);
    }
}

void brdgmng::check_first_actor_permission(const name& actor, const uint64_t permission_id) {
    require_auth(actor);
    auto permission_itr = _permission.require_find(permission_id, "brdgmng.xsat::get_first_actor: permission id does not exists");
    name first_actor = permission_itr->actors[0];
    check(actor == first_actor, "brdgmng.xsat::get_first_actor: only the first actor can perform this operation");
}

void brdgmng::check_permission(const name& actor, const uint64_t permission_id) {
    require_auth(actor);
    auto permission_itr = _permission.require_find(permission_id, "brdgmng.xsat::check_permission: permission id does not exists");
    bool has_permission = false;
    for (const auto& item : permission_itr->actors) {
        if (actor == item) {
            has_permission = true;
            break;
        }
    }
    check(has_permission, "brdgmng.xsat::check_permission: actor does not have permission");
}

uint64_t brdgmng::next_address_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.address_id++;
    _global_id.set(global_id, get_self());
    return global_id.address_id;
}

uint64_t brdgmng::next_address_mapping_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.address_mapping_id++;
    _global_id.set(global_id, get_self());
    return global_id.address_mapping_id;
}

uint64_t brdgmng::next_deposit_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.deposit_id++;
    _global_id.set(global_id, get_self());
    return global_id.deposit_id;
}

uint64_t brdgmng::next_withdraw_id() {
    global_id_row global_id = _global_id.get_or_default();
    global_id.withdraw_id++;
    _global_id.set(global_id, get_self());
    return global_id.withdraw_id;
}

bool brdgmng::is_final_address_status(const address_status address_status) {
    return address_status == address_status_confirmed || address_status == address_status_failed;
}

bool brdgmng::is_final_tx_status(const tx_status tx_status) {
    return tx_status == tx_status_confirmed || tx_status == tx_status_failed || tx_status == tx_status_rollbacked;
}

bool brdgmng::is_final_order_status(const order_status order_status) {
    return order_status == order_status_partially_failed || order_status == order_status_failed || order_status == order_status_finished ||
           order_status == order_status_canceled;
}

bool brdgmng::verify_providers(const std::vector<name>& requested_actors, const std::vector<name>& provider_actors) {
    for (const auto& request : requested_actors) {
        if (std::find(provider_actors.begin(), provider_actors.end(), request) == provider_actors.end()) {
            return false;
        }
    }
    return true;
}

bool brdgmng::check_utxo_exist(const checksum256& tx_id, const uint32_t index) {
    const checksum256 utxo_id = xsat::utils::compute_utxo_id(tx_id, index);
    utxo_manage::utxo_table _utxo(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    auto utxo_id_idx = _utxo.get_index<"byutxoid"_n>();
    auto utxo_id_itr = utxo_id_idx.find(utxo_id);
    if (utxo_id_itr != utxo_id_idx.end()) {
        return true;
    }

    utxo_manage::pending_utxo_table _pending_utxo(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    auto pending_utxo_id_idx = _pending_utxo.get_index<"byutxoid"_n>();
    auto pending_utxo_id_itr = pending_utxo_id_idx.find(utxo_id);
    if (pending_utxo_id_itr != pending_utxo_id_idx.end()) {
        return true;
    }

    utxo_manage::spent_utxo_table _spent_utxo(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    auto spent_utxo_id_idx = _spent_utxo.get_index<"byutxoid"_n>();
    auto spent_utxo_id_itr = spent_utxo_id_idx.find(utxo_id);
    if (spent_utxo_id_itr != spent_utxo_id_idx.end()) {
        return true;
    }

    return false;
}

void brdgmng::token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});
    transfer.send(from, to, value.quantity, memo);
}

void brdgmng::handle_btc_deposit(const uint64_t permission_id, const uint64_t amount, const checksum160& evm_address) {
    // issue BTC
    asset quantity = asset(amount, BTC_SYMBOL);
    btc::issue_action issue(BTC_CONTRACT, {BTC_CONTRACT, "active"_n});
    issue.send(BTC_CONTRACT, quantity, "issue BTC to evm address");
    // transfer BTC to evm address
    btc::transfer_action transfer(BTC_CONTRACT, {BTC_CONTRACT, "active"_n});
    transfer.send(BTC_CONTRACT, EVM_CONTRACT, quantity, "0x" + xsat::utils::sha1_to_hex(evm_address));

    statistics_table _statistics = statistics_table(_self, permission_id);
    statistics_row statistics = _statistics.get_or_default();
    statistics.total_deposit_amount += amount;
    _statistics.set(statistics, get_self());
}

void brdgmng::handle_btc_withdraw(const uint64_t permission_id, const uint64_t amount) {
    // transfer BTC btc.xsat
    asset quantity = asset(amount, BTC_SYMBOL);
    btc::transfer_action transfer(BTC_CONTRACT, {get_self(), "active"_n});
    transfer.send(get_self(), BTC_CONTRACT, quantity, "transfer BTC to btc.xsat");
    // retire BTC
    btc::retire_action retire(BTC_CONTRACT, {BTC_CONTRACT, "active"_n});
    retire.send(quantity, "retire BTC from withdraw");

    statistics_table _statistics = statistics_table(_self, permission_id);
    statistics_row statistics = _statistics.get_or_default();
    statistics.total_withdraw_amount += amount;
    _statistics.set(statistics, get_self());
}

string brdgmng::generate_order_no(const std::vector<uint64_t>& ids) {
    auto timestamp = current_time_point().sec_since_epoch();
    std::vector<uint64_t> sorted_ids = ids;
    std::sort(sorted_ids.begin(), sorted_ids.end());
    std::string order_no = std::to_string(timestamp) + "-";
    for (size_t i = 0; i < sorted_ids.size(); ++i) {
        order_no += std::to_string(sorted_ids[i]);
        if (i < sorted_ids.size() - 1) {
            order_no += "-";
        }
    }
    checksum160 order_no_hash = xsat::utils::hash_ripemd160(order_no);
    return xsat::utils::checksum160_to_string(order_no_hash);
}
