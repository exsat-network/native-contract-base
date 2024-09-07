#include <brdgmng.xsat/brdgmng.xsat.hpp>

#ifdef DEBUG
#include <bitcoin/script/address.hpp>
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
void brdgmng::addperm(const uint64_t id, const vector<name> actors) {
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
void brdgmng::addaddresses(const name actor, const uint64_t permission_id, string b_id, string wallet_code, const vector<string> btc_addresses) {
    require_auth(actor);
    if (btc_addresses.empty()) {
        check(false, "brdgmng.xsat::addaddresses: btc_addresses cannot be empty");
    }
    auto permission_itr = _permission.require_find(permission_id, "brdgmng.xsat::addaddresses: permission id does not exists");
    bool has_permission = false;
    for (const auto& item : permission_itr->actors) {
        if (actor == item) {
            has_permission = true;
            break;
        }
    }
    check(has_permission, "brdgmng.xsat::addaddresses: actor does not have permission");

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
            row.btc_address = btc_address;
            row.status = initialized;
            });
    }
    statistics_row statistics = _statistics.get_or_default();
    statistics.total_btc_address_count += btc_addresses.size();
    _statistics.set(statistics, get_self());
}

[[eosio::action]]
void brdgmng::confirmaddr(const name actor, const uint64_t permission_id, const string btc_addresses) {
    require_auth(actor);
    auto permission_itr = _permission.require_find(permission_id, "brdgmng.xsat::confirmaddr: permission id does not exists");
    bool has_permission = false;
    for (const auto& item : permission_itr->actors) {
        if (actor == item) {
            has_permission = true;
            break;
        }
    }
    check(has_permission, "brdgmng.xsat::confirmaddr: actor does not have permission");

    auto address_idx = _address.get_index<"bybtcaddr"_n>();
    checksum256 btc_addrress_hash = xsat::utils::hash(btc_addresses);
    auto address_itr = address_idx.require_find(btc_addrress_hash, "brdgmng.xsat::confirmaddr: btc_address does not exists in address");
    check(address_itr->status != confirmed, "brdgmng.xsat::confirmaddr: btc_address status is already confirmed");

    auto actor_itr = std::find_if(address_itr->provider_actors.begin(),
        address_itr->provider_actors.end(), [&](const auto& u) {
            return u == actor;
        });
    check(actor_itr == address_itr->provider_actors.end(), "brdgmng.xsat::confirmaddr: actor already exists in the provider actors list");
    address_idx.modify(address_itr, same_payer, [&](auto& row) {
        row.provider_actors.push_back(actor);
        if (verifyProviders(permission_itr->actors, address_itr->provider_actors)) {
            row.status = confirmed;
        }
    });
}

[[eosio::action]]
void brdgmng::mappingaddr(const name actor, const uint64_t permission_id, const checksum160 evm_address) {
    require_auth(actor);
    auto permission_itr = _permission.require_find(permission_id, "brdgmng.xsat::confirmaddr: permission id does not exists");
    bool has_permission = false;
    for (const auto& item : permission_itr->actors) {
        if (actor == item) {
            has_permission = true;
            break;
        }
    }
    check(has_permission, "brdgmng.xsat::mappingaddr: actor does not have permission");

    // auto address_idx = _address.get_index<"bybtcaddr"_n>();
    auto evm_address_mapping_idx = _address_mapping.get_index<"byevmaddr"_n>();
    checksum256 evm_address_hash = xsat::utils::compute_id(evm_address);
    auto evm_address_mapping_itr = evm_address_mapping_idx.find(evm_address_hash);
    check(evm_address_mapping_itr == evm_address_mapping_idx.end(), "brdgmng.xsat::mappingaddr: evm_address already exists in address_mapping");

    //从address根据permission_id取出btc_address，需要状态是confirmed的地址
    auto address_idx = _address.get_index<"bypermission"_n>();
    auto address_lower = address_idx.lower_bound(permission_id);
    auto address_upper = address_idx.upper_bound(permission_id);
    check(address_lower != address_upper, "brdgmng.xsat::mappingaddr: permission id does not exists in address");
    auto address_itr = address_lower;
    string btc_address;
    for (; address_itr != address_upper; ++address_itr) {
        if (address_itr->status == confirmed) {
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

bool brdgmng::verifyProviders(std::vector<name> requested_actors, std::vector<name> provider_actors) {
    for (const auto& request : requested_actors) {
        if (std::find(provider_actors.begin(), provider_actors.end(), request) == provider_actors.end()) {
            return false;
        }
    }
    return true;
}