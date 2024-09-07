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

    if (table_name == "global_id"_n) {
        _global.remove();
    } else if (table_name == "custodies"_n) {
        brdgmng_index _brdgmng(get_self(), get_self().value);
        clear_table(_brdgmng, rows_to_clear);
    } else if (table_name == "vaults"_n) {
        vault_index _vault(get_self(), get_self().value);
        clear_table(_vault, rows_to_clear);
    } else {
        check(false, "brdgmng::cleartable: [table_name] unknown table to clear");
    }
}
