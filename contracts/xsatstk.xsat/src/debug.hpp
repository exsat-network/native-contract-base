template <typename T>
void xsat_stake::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void xsat_stake::cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? scope->value : get_self().value;

    // tables
    xsat_stake::release_table _release(get_self(), value);
    xsat_stake::staking_table _staking(get_self(), value);

    if (table_name == "globalid"_n)
        _global_id.remove();
    else if (table_name == "config"_n)
        _config.remove();
    else if (table_name == "staking"_n)
        clear_table(_staking, rows_to_clear);
    else if (table_name == "releases"_n)
        clear_table(_release, rows_to_clear);
    else
        check(false, "xsatstk.xsat::cleartable: [table_name] unknown table to clear");
}
