#include <rescmng.xsat/rescmng.xsat.hpp>
#include <btc.xsat/btc.xsat.hpp>
#include "../internal/utils.hpp"

//@auth get_self()
[[eosio::action]]
void resource_management::init(const name& fee_account, const asset& cost_per_slot, const asset& cost_per_upload,
                               const asset& cost_per_verification, const asset& cost_per_endorsement,
                               const asset& cost_per_parse) {
    require_auth(get_self());

    check(is_account(fee_account), "rescmng.xsat::init: fee_account does not exists");
    check(cost_per_slot.symbol == BTC_SYMBOL, "rescmng.xsat::init: cost_per_slot symbol must be 8,BTC");
    check(cost_per_upload.symbol == BTC_SYMBOL, "rescmng.xsat::init: cost_per_upload symbol must be 8,BTC");
    check(cost_per_verification.symbol == BTC_SYMBOL, "rescmng.xsat::init: cost_per_verification symbol must be 8,BTC");
    check(cost_per_endorsement.symbol == BTC_SYMBOL, "rescmng.xsat::init: cost_per_endorsement symbol must be 8,BTC");
    check(cost_per_parse.symbol == BTC_SYMBOL, "rescmng.xsat::init: cost_per_parse symbol must be 8,BTC");

    auto config = _config.get_or_default();
    config.cost_per_upload = cost_per_upload;
    config.cost_per_slot = cost_per_slot;
    config.cost_per_verification = cost_per_verification;
    config.cost_per_endorsement = cost_per_endorsement;
    config.cost_per_parse = cost_per_parse;
    config.fee_account = fee_account;
    _config.set(config, get_self());
}

//@auth get_self()
[[eosio::action]]
void resource_management::setstatus(const bool disabled_withdraw) {
    require_auth(get_self());

    auto config = _config.get_or_default();
    config.disabled_withdraw = disabled_withdraw;
    _config.set(config, get_self());
}

//@auth blksync.xsat or blkendt.xsat or utxomng.xsat or poolreg.xsat or blkendt.xsat
[[eosio::action]]
void resource_management::pay(const uint64_t height, const checksum256& hash, const name& owner, const fee_type type,
                              const uint64_t quantity) {
    check(has_auth(BLOCK_SYNC_CONTRACT) || has_auth(BLOCK_ENDORSE_CONTRACT) || has_auth(UTXO_MANAGE_CONTRACT)
              || has_auth(POOL_REGISTER_CONTRACT),
          "rescmng.xsat::pay: missing auth [blksync.xsat/blkendt.xsat/utxmng.xsat/poolreg.xsat]");

    check(quantity > 0, "rescmng.xsat::pay: must pay positive quantity");

    auto fee_amount = get_fee(type, quantity);
    auto account_itr = _account.find(owner.value);
    check(account_itr != _account.end() && account_itr->balance >= fee_amount,
          "rescmng.xsat::pay: insufficient balance");

    _account.modify(account_itr, same_payer, [&](auto& row) {
        row.balance -= fee_amount;
    });
    auto config = _config.get();
    if (fee_amount.amount > 0) {
        token_transfer(get_self(), config.fee_account, {fee_amount, BTC_CONTRACT}, "fee deduction");
    }

    // log
    resource_management::paylog_action _paylog(get_self(), {get_self(), "active"_n});
    _paylog.send(height, hash, owner, type, fee_amount);
}

//@auth owner
[[eosio::action]]
void resource_management::withdraw(const name& owner, const asset& quantity) {
    require_auth(owner);

    auto config = _config.get();
    check(!config.disabled_withdraw, "rescmng.xsat::withdraw: withdraw balance status is disabled");

    auto account_itr = _account.find(owner.value);
    check(account_itr != _account.end() && account_itr->balance >= quantity,
          "rescmng.xsat::withdraw: no balance to withdraw");
    auto balance = account_itr->balance - quantity;
    if (balance.amount > 0) {
        _account.modify(account_itr, same_payer, [&](auto& row) {
            row.balance -= quantity;
        });
    } else {
        _account.erase(account_itr);
    }

    token_transfer(get_self(), owner, {quantity, BTC_CONTRACT}, "withdraw");

    // log
    resource_management::withdrawlog_action _withdrawlog(get_self(), {get_self(), "active"_n});
    _withdrawlog.send(owner, quantity, balance);
}

[[eosio::on_notify("*::transfer")]]
void resource_management::on_transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    // ignore transfers
    if (to != get_self()) return;

    const name contract = get_first_receiver();
    check(contract == BTC_CONTRACT && quantity.symbol == BTC_SYMBOL, "rescmng.xsat: only transfer [btc.xsat/BTC]");
    do_deposit(from, contract, quantity, memo);
}

void resource_management::do_deposit(const name& from, const name& contract, const asset& quantity,
                                     const string& memo) {
    check(quantity.amount > 0, "rescmng.xsat: must transfer positive quantity");
    auto receiver = xsat::utils::parse_name(memo);
    check(receiver && is_account(receiver), "rescmng.xsat: invalid memo, ex: \"<receiver>\"");
    auto account_itr = _account.find(receiver.value);
    if (account_itr == _account.end()) {
        account_itr = _account.emplace(get_self(), [&](auto& row) {
            row.owner = receiver;
            row.balance = quantity;
        });
    } else {
        _account.modify(account_itr, same_payer, [&](auto& row) {
            row.balance += quantity;
        });
    }

    // log
    resource_management::depositlog_action _depositlog(get_self(), {get_self(), "active"_n});
    _depositlog.send(from, quantity, account_itr->balance);
}

asset resource_management::get_fee(const fee_type type, const uint64_t quantity) {
    auto config = _config.get_or_default();
    switch (type) {
        case BUY_SLOT:
            return config.cost_per_slot * quantity;
        case PUSH_CHUNK:
            return config.cost_per_upload * quantity;
        case VERIFY:
            return config.cost_per_verification * quantity;
        case ENDORSE:
            return config.cost_per_endorsement * quantity;
        case PARSE:
            return config.cost_per_parse * quantity;
        default:
            check(false, "rescmng.xsat::get_fee: invalid type");
    }
}

void resource_management::token_transfer(const name& from, const name& to, const extended_asset& value,
                                         const string& memo) {
    btc::transfer_action transfer(value.contract, {from, "active"_n});
    transfer.send(from, to, value.quantity, memo);
}