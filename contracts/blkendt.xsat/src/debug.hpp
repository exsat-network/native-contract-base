template <typename T>
void block_endorse::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void block_endorse::cleartable(const name table_name, const optional<uint64_t> scope,
                               const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? *scope : get_self().value;

    endorsement_table _endorsement(get_self(), value);

    if (table_name == "endorsements"_n)
        clear_table(_endorsement, rows_to_clear);
    else if (table_name == "config"_n)
        _config.remove();
    else
        check(false, "blkendt.xsat::cleartable: [table_name] unknown table to clear");
}
