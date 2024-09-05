template <typename T>
void endorse_manage::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void endorse_manage::cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? scope->value : get_self().value;

    endorse_manage::whitelist_table _whitelist(get_self(), value);
    evm_proxy_table _evm_proxy = evm_proxy_table(get_self(), value);

    if (table_name == "globalid"_n)
        _global_id.remove();
    else if (table_name == "stat"_n)
        _stat.remove();
    else if (table_name == "validators"_n)
        clear_table(_validator, rows_to_clear);
    else if (table_name == "stakers"_n)
        clear_table(_native_stake, rows_to_clear);
    else if (table_name == "evmstakers"_n)
        clear_table(_evm_stake, rows_to_clear);
    else if (table_name == "evmproxys"_n)
        clear_table(_evm_proxy, rows_to_clear);
    else if (table_name == "whitelist"_n)
        clear_table(_whitelist, rows_to_clear);
    else
        check(false, "endrmng.xsat::cleartable: [table_name] unknown table to clear");
}
