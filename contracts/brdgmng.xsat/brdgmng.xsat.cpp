#include <brdgmng.xsat/brdgmng.xsat.hpp>

#ifdef DEBUG
// #include <bitcoin/script/address.hpp>
#include "./src/debug.hpp"
#endif

[[eosio::action]]
void brdgmng::initialize() {
    require_auth(BOOT_ACCOUNT);
    boot_row boot = _boot.get_or_default();
    check(!boot.initialized, "brdgmng.xsat::boot: already initialized");
    // initialize issue 1 BTC
    btc::issue_action issue(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
    asset quantity = asset(BTC_BASE, BTC_SYMBOL);
    issue.send(BTC_CONTRACT, quantity, "initialize issue one BTC to boot.xsat");
    // transfer BTC to boot.xsat
    btc::transfer_action transfer(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
    transfer.send(BTC_CONTRACT, BOOT_ACCOUNT, quantity, "transfer one BTC to boot.xsat");
    boot.initialized = true;
    boot.returned = false;
    boot.quantity = quantity;
    _boot.set(boot, get_self());
}

[[eosio::action]]
void brdgmng::addperm(const uint64_t id, const vector<name>& actors) {
    require_auth(get_self());
    if (actors.empty()) {
        check(false, "brdgmng.xsat::addperm: actors cannot be empty");
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
    require_auth(actor);
    if (btc_addresses.empty()) {
        check(false, "brdgmng.xsat::addaddresses: btc_addresses cannot be empty");
    }
    check_permission(actor, permission_id);

    auto address_idx = _address.get_index<"bybtcaddr"_n>();
    auto address_mapping_idx = _address_mapping.get_index<"bybtcaddr"_n>();
    for (const auto& btc_address : btc_addresses) {
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
            row.provider_actors = vector<name> { actor };
            row.statuses = vector<string> { "initiated" };
            row.confirmed_count = 1;
            row.status = global_status_initiated;
            row.btc_address = btc_address;
        });
    }
    statistics_row statistics = _statistics.get_or_default();
    statistics.total_btc_address_count += btc_addresses.size();
    _statistics.set(statistics, get_self());
}

[[eosio::action]]
void brdgmng::valaddress(const name& actor, const uint64_t permission_id, const uint64_t address_id, const string& status) {
    check_permission(actor, permission_id);

    auto address_itr = _address.require_find(address_id, "brdgmng.xsat::valaddress: address_id does not exists in address");
    check(address_itr->status != global_status_confirmed, "brdgmng.xsat::valaddress: btc_address status is already confirmed");

    auto actor_itr = std::find_if(address_itr->provider_actors.begin(),
        address_itr->provider_actors.end(), [&](const auto& u) {
            return u == actor;
        });
    check(actor_itr == address_itr->provider_actors.end(), "brdgmng.xsat::valaddress: actor already exists in the provider actors list");
    _address.modify(address_itr, same_payer, [&](auto& row) {
        row.provider_actors.push_back(actor);
        row.statuses.push_back(status);
        if (status == "confirmed") {
            row.confirmed_count++;
        }
        if (verifyProviders(_permission.get(permission_id).actors, address_itr->provider_actors)) {
            if (row.confirmed_count == address_itr->provider_actors.size()) {
                row.status = global_status_confirmed;
            } else {
                row.status = global_status_failed;
            }
        }
    });
}

[[eosio::action]]
void brdgmng::mappingaddr(const name& actor, const uint64_t permission_id, const checksum160 evm_address) {
    check_permission(actor, permission_id);

    auto evm_address_mapping_idx = _address_mapping.get_index<"byevmaddr"_n>();
    checksum256 evm_address_hash = xsat::utils::compute_id(evm_address);
    auto evm_address_mapping_itr = evm_address_mapping_idx.find(evm_address_hash);
    check(evm_address_mapping_itr == evm_address_mapping_idx.end(), "brdgmng.xsat::mappingaddr: evm_address already exists in address_mapping");

    // get the btc_address from the address according to the permission_id, which needs to be a confirmed address
    auto address_idx = _address.get_index<"bypermission"_n>();
    auto address_lower = address_idx.lower_bound(permission_id);
    auto address_upper = address_idx.upper_bound(permission_id);
    check(address_lower != address_upper, "brdgmng.xsat::mappingaddr: permission id does not exists in address");
    auto address_itr = address_lower;
    string btc_address;
    for (; address_itr != address_upper; ++address_itr) {
        if (address_itr->status == global_status_confirmed) {
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

    statistics_row statistics = _statistics.get_or_default();
    statistics.mapped_address_count++;
    _statistics.set(statistics, get_self());
}

[[eosio::action]]
void brdgmng::deposit(const name& actor, const uint64_t permission_id, const string& b_id, const string& wallet_code, const string& btc_address, const string& order_no,
        const uint64_t block_height, const string& tx_id, const uint64_t amount, const string& tx_status, const optional<string>& remark_detail, const uint64_t tx_time_stamp, const uint64_t create_time_stamp) {
    check_permission(actor, permission_id);
    check(_config.get_or_default().deposit_enable, "brdgmng.xsat::deposit: deposit is disabled");

    //todo Determine whether the order_no is unique
    _deposit.emplace(get_self(), [&](auto& row) {
        row.id = next_deposit_id();
        row.permission_id = permission_id;
        row.provider_actors = vector<name> { actor };
        row.tx_statuses = vector<string> { tx_status };
        row.confirmed_count = 1;
        row.status = global_status_initiated;
        row.b_id = b_id;
        row.wallet_code = wallet_code;
        row.btc_address = btc_address;
        row.order_no = order_no;
        row.block_height = block_height;
        row.tx_id = tx_id;
        row.amount = amount;
        if (remark_detail.has_value()) {
            row.remark_detail = *remark_detail;
        }
        row.tx_time_stamp = tx_time_stamp;
        row.create_time_stamp = create_time_stamp;
    });
}

[[eosio::action]]
void brdgmng::valdeposit(const name& actor, const uint64_t permission_id, const uint64_t deposit_id, const string& tx_status, const optional<string>& remark_detail) {
    check_permission(actor, permission_id);
    auto deposit_itr = _deposit.require_find(deposit_id, "brdgmng.xsat::valdeposit: deposit id does not exists");
    check(deposit_itr->status != global_status_confirmed, "brdgmng.xsat::valdeposit: deposit status is already confirmed");
    check(deposit_itr->status != global_status_failed, "brdgmng.xsat::valdeposit: deposit status is already failed");

    auto actor_itr = std::find_if(deposit_itr->provider_actors.begin(),
        deposit_itr->provider_actors.end(), [&](const auto& u) {
            return u == actor;
        });
    check(actor_itr == deposit_itr->provider_actors.end(), "brdgmng.xsat::valdeposit: actor already exists in the provider actors list");
    _deposit.modify(deposit_itr, same_payer, [&](auto& row) {
        row.provider_actors.push_back(actor);
        row.tx_statuses.push_back(tx_status);
        if (tx_status == "confirmed") {
            row.confirmed_count++;
        }
        if (remark_detail.has_value()) {
            row.remark_detail = *remark_detail;
        }
        if (verifyProviders(_permission.get(permission_id).actors, deposit_itr->provider_actors)) {
            if (row.confirmed_count == deposit_itr->provider_actors.size()) {
                row.status = global_status_confirmed;
                handle_btc_deposit(deposit_itr->amount, deposit_itr->evm_address);
            } else {
                row.status = global_status_failed;
            }
        }
    });
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
    btc::transfer_action transfer(contract, { get_self(), "active"_n });
    transfer.send(get_self(), BTC_CONTRACT, quantity, "return BTC to btc.xsat");
    btc::retire_action retire(contract, { BTC_CONTRACT, "active"_n });
    retire.send(quantity, "retire BTC from boot.xsat");

    boot.returned = true;
    _boot.set(boot, get_self());
}

void brdgmng::do_withdraw(const name& from, const name& contract, const asset& quantity, const string& memo) {
    check(_config.get_or_default().withdraw_enable, "brdgmng.xsat: withdraw is disabled");
    check(quantity.amount > 0, "brdgmng.xsat: must transfer positive quantity");
    auto parts = xsat::utils::split(memo, ",");
    auto INVALID_MEMO = "brdgmng.xsat: invalid memo ex: \"<btc_address>,<gas_level>\"";
    check(parts.size() == 2, INVALID_MEMO);
    string btc_address = parts[0];
    string gas_level = parts[1]; //fast, avg, slow
    string order_no = "";
    uint64_t withdraw_id = next_withdraw_id();
    if (gas_level == "fast") {
        order_no = generate_order_no({withdraw_id});
    }
    _withdraw.emplace(get_self(), [&](auto& row) {
        row.id = withdraw_id;
        row.btc_address = btc_address;
        row.amount = quantity.amount;
        row.gas_level = gas_level;
        row.status = global_status_initiated;
        if (!order_no.empty()) {
            row.order_no = order_no;
        }
    });
}

//得根据订单号来批量处理
[[eosio::action]]
void brdgmng::valwithdraw(const name& actor, const uint64_t permission_id, const uint64_t withdraw_id, const string& b_id, const string& wallet_code,
        const uint64_t block_height, const string& tx_id, const string& tx_status, const optional<string>& remark_detail, const uint64_t tx_time_stamp, const uint64_t create_time_stamp) {
    check_permission(actor, permission_id);
    check_permission(actor, permission_id);
    auto withdraw_itr = _withdraw.require_find(withdraw_id, "brdgmng.xsat::valwithdraw: withdraw id does not exists");
    check(withdraw_itr->status != global_status_initiated, "brdgmng.xsat::valwithdraw: withdraw status is already confirmed");
    check(withdraw_itr->status != global_status_failed, "brdgmng.xsat::valwithdraw: withdraw status is failed");

    auto actor_itr = std::find_if(withdraw_itr->provider_actors.begin(),
        withdraw_itr->provider_actors.end(), [&](const auto& u) {
            return u == actor;
        });
    check(actor_itr == withdraw_itr->provider_actors.end(), "brdgmng.xsat::confirmwithdraw: actor already exists in the provider actors list");
    _withdraw.modify(withdraw_itr, same_payer, [&](auto& row) {
        row.provider_actors.push_back(actor);
        row.tx_statuses.push_back(tx_status);
        if (tx_status == "confirmed") {
            row.confirmed_count++;
        }
        if (verifyProviders(_permission.get(permission_id).actors, withdraw_itr->provider_actors)) {
            if (row.confirmed_count == withdraw_itr->provider_actors.size()) {
                row.status = global_status_confirmed;
                if (withdraw_itr->amount >= _config.get_or_default().limit_amount) {
                    // issue BTC
                    asset quantity = asset(withdraw_itr->amount, BTC_SYMBOL);
                    btc::issue_action issue(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
                    issue.send(BTC_CONTRACT, quantity, "issue BTC to evm address");
                    // transfer BTC to evm address
                    btc::transfer_action transfer(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
                    transfer.send(BTC_CONTRACT, ERC20_CONTRACT, quantity, "0x" + xsat::utils::sha1_to_hex(withdraw_itr->evm_address));
                }
            } else {
                row.status = global_status_failed;
            }
        }
    });
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

bool brdgmng::verifyProviders(const std::vector<name>& requested_actors, const std::vector<name>& provider_actors) {
    for (const auto& request : requested_actors) {
        if (std::find(provider_actors.begin(), provider_actors.end(), request) == provider_actors.end()) {
            return false;
        }
    }
    return true;
}

void brdgmng::token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});
    transfer.send(from, to, value.quantity, memo);
}

void brdgmng::handle_btc_deposit(const uint64_t amount, const checksum160& evm_address) {
    if (amount >= _config.get_or_default().limit_amount) {
        const uint64_t issue_amount = safemath::sub(amount, _config.get_or_default().deposit_fee);
        // issue BTC
        asset quantity = asset(issue_amount, BTC_SYMBOL);
        btc::issue_action issue(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
        issue.send(BTC_CONTRACT, quantity, "issue BTC to evm address");
        // transfer BTC to evm address
        btc::transfer_action transfer(BTC_CONTRACT, { BTC_CONTRACT, "active"_n });
        transfer.send(BTC_CONTRACT, ERC20_CONTRACT, quantity, "0x" + xsat::utils::sha1_to_hex(evm_address));

        statistics_row statistics = _statistics.get_or_default();
        statistics.total_deposit_amount += issue_amount;
        _statistics.set(statistics, get_self());
    }
}

string brdgmng::generate_order_no(const std::vector<uint64_t>& ids) {
    auto timestamp = current_time_point().sec_since_epoch();
    std::vector<uint64_t> sorted_ids = ids;
    std::sort(sorted_ids.begin(), sorted_ids.end());
    std::string order_no;
    for (size_t i = 0; i < sorted_ids.size(); ++i) {
        order_no += std::to_string(sorted_ids[i]);
        if (i < sorted_ids.size() - 1) {
            order_no += "-";
        }
    }
    order_no += "-" + std::to_string(timestamp); // 添加时间戳
    return order_no;
}