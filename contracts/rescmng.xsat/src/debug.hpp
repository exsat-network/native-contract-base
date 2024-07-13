template <typename T>
void resource_management::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void resource_management::cleartable(const name table_name, const optional<name> scope,
                                     const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? scope->value : get_self().value;

    // tables

    if (table_name == "config"_n)
        _config.remove();
    else if (table_name == "accounts"_n)
        clear_table(_account, rows_to_clear);
    else
        check(false, "resource_management::cleartable: [table_name] unknown table to clear");
}
