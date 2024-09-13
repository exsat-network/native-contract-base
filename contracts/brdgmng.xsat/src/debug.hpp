template <typename T>
void brdgmng::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void brdgmng::cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? scope->value : get_self().value;

    if (table_name == "boot"_n) {
        _boot.remove();
    } else if (table_name == "globalid"_n) {
        _global_id.remove();
    } else if (table_name == "configs"_n) {
        _config.remove();
    } else if (table_name == "statistics"_n) {
        _statistics.remove();
    } else if (table_name == "permissions"_n) {
        permission_index _permissions(get_self(), get_self().value);
        clear_table(_permissions, rows_to_clear);
    } else if (table_name == "addresses"_n) {
        address_index _address(get_self(), get_self().value);
        clear_table(_address, rows_to_clear);
    } else if (table_name == "addrmappings"_n) {
        address_mapping_index _address_mapping(get_self(), get_self().value);
        clear_table(_address_mapping, rows_to_clear);
    } else if (table_name == "deposits"_n) {
        check(scope.has_value(), "brdgmng::cleartable: [scope] is required for deposits table");
        deposit_index _deposit(get_self(), scope->value);
        clear_table(_deposit, rows_to_clear);
    } else if (table_name == "withdraws"_n) {
        check(scope.has_value(), "brdgmng::cleartable: [scope] is required for withdraws table");
        withdraw_index _withdraw(get_self(), scope->value);
        clear_table(_withdraw, rows_to_clear);
    } else {
        check(false, "brdgmng::cleartable: [table_name] unknown table to clear");
    }
}
