template <typename T>
void custody::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void custody::cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? scope->value : get_self().value;

    if (table_name == "globalid"_n) {
        _global.remove();
    } else if (table_name == "custodies"_n) {
        custody_index _custody(get_self(), get_self().value);
        clear_table(_custody, rows_to_clear);
    } else {
        check(false, "custody::cleartable: [table_name] unknown table to clear");
    }
}

[[eosio::action]]
void custody::addr2pubkey(const string& address) {
    auto scriptpubkey = bitcoin::utility::address_to_scriptpubkey(address);
    auto res = xsat::utils::bytes_to_hex(scriptpubkey);
    print_f("scriptpubkey result: %", res);
}

[[eosio::action]]
void custody::pubkey2addr(const vector<uint8_t> data) {
    std::vector<string> res;
    bool success = bitcoin::ExtractDestination(data, res);
    if (success) {
        print_f("address: %", res[0]);
    } else {
        print_f("failed to extract address");
    }
}

[[eosio::action]]
void custody::updateheight(const uint64_t height) {
    require_auth(get_self());
    global_row global = _global.get_or_default();
    global.last_height = height;
    _global.set(global, get_self());
}


