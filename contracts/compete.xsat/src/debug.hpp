template <typename T>
void compete::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void compete::cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? scope->value : get_self().value;

    if (table_name == "globals"_n) {
        _global.remove();
    } else if (table_name == "rounds"_n) {
        clear_table(_round, rows_to_clear);
    } else if (table_name == "activations"_n) {
        clear_table(_activation, rows_to_clear);
    } else {
        check(false, "compete::cleartable: [table_name] unknown table to clear");
    }
}
