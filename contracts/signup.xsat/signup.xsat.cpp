#include <signup.xsat/signup.xsat.hpp>

[[eosio::action]]
void signup::config(const name& recharge_account, const asset& stake_net, const asset& stake_cpu, const uint64_t buy_ram_bytes) {
    require_auth(get_self());
    config_row config = _config.get_or_default();
    config.recharge_account = recharge_account;
    config.stake_net = stake_net;
    config.stake_cpu = stake_cpu;
    config.buy_ram_bytes = buy_ram_bytes;
    _config.set(config, get_self());
}

[[eosio::action]]
void signup::addtoken(const name& contract, const asset& cost) {
    require_auth(get_self());
    auto token_itr = _token.find(contract.value);
    if (token_itr == _token.end()) {
        _token.emplace(get_self(), [&](auto& row) {
            row.contract = contract;
            row.cost = cost;
        });
    } else {
        _token.modify(token_itr, get_self(), [&](auto& row) { row.cost = cost; });
    }
}

[[eosio::action]]
void signup::deltoken(const name& contract) {
    require_auth(get_self());
    auto token_itr = _token.find(contract.value);
    check(token_itr != _token.end(), "deltoken: token not found");
    _token.erase(token_itr);
}

[[eosio::on_notify("*::transfer")]]
void signup::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    if (from == _self || to != _self) {
        return;
    }

    if (from == _config.get_or_default().recharge_account) {
        return;
    }

    const name contract = get_first_receiver();
    auto token_itr = _token.require_find(contract.value, "token is not supported");

    check(quantity.is_valid(), "invalid quantity");
    check(quantity > token_itr->cost, "insufficient balance");

    auto parts = xsat::utils::split(memo, "-");
    check(parts.size() >= 2, "invalid memo ex: <account_name>-<owner_key>-<active_key>");
    std::string account_name_str = parts[0];
    check(account_name_str.length() <= 12, "invalid account name");
    name new_account_name = name(account_name_str);
    check(!is_account(new_account_name), "account name already exists");

    std::string owner_key_str;
    std::string active_key_str;
    if (parts.size() == 2) {
        owner_key_str = parts[1];
        active_key_str = parts[1];
    } else {
        owner_key_str = parts[1];
        active_key_str = parts[2];
    }

    std::string pubkey_prefix("EOS");

    std::array<unsigned char, 33> pubkey_data_o = validate_key(owner_key_str);
    std::array<unsigned char, 33> pubkey_data_a = validate_key(active_key_str);

    // owner
    signup_public_key pubkey_o = {
        .type = 0,
        .data = pubkey_data_o,
    };

    key_weight pubkey_weight_o = {
        .key = pubkey_o,
        .weight = 1,
    };

    authority owner = {.threshold = 1, .keys = {pubkey_weight_o}, .accounts = {}, .waits = {}};

    // active
    signup_public_key pubkey_a = {
        .type = 0,
        .data = pubkey_data_a,
    };

    key_weight pubkey_weight_a = {
        .key = pubkey_a,
        .weight = 1,
    };

    authority active = {.threshold = 1, .keys = {pubkey_weight_a}, .accounts = {}, .waits = {}};
    newaccount new_account = {.creator = _self, .name = new_account_name, .owner = owner, .active = active};
    permission_level self_active = {get_self(), name("active")};

    // Create new account
    action(self_active, EOSIO, name("newaccount"), new_account).send();

    config_row config = _config.get_or_default();

    // Buy RAM
    std::tuple buyram_data = std::make_tuple(_self, new_account_name, config.buy_ram_bytes);
    action(self_active, EOSIO, name("buyrambytes"), buyram_data).send();

    // Delegate CPU and NET
    std::tuple dbw_data = std::make_tuple(_self, new_account_name, config.stake_net, config.stake_cpu, true);
    action(self_active, EOSIO, name("delegatebw"), dbw_data).send();

    // Transfer remaining balance to new account
    std::string new_memo = "Welcome to exSat: https://exsat.network";
    std::tuple transfer_data = std::make_tuple(_self, new_account_name, quantity - token_itr->cost, new_memo);
    action(self_active, token_itr->contract, name("transfer"), transfer_data).send();
}
