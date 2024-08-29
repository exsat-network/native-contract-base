template <typename T>
void utxo_manage::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void utxo_manage::cleartable(const name table_name, const optional<uint64_t> scope, const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? *scope : get_self().value;

    // tables
    utxo_manage::block_extra_table _block_extra(get_self(), value);

    if (table_name == "utxos"_n)
        clear_table(_utxo, rows_to_clear);
    else if (table_name == "pendingutxos"_n)
        clear_table(_pending_utxo, rows_to_clear);
    else if (table_name == "spentutxos"_n)
        clear_table(_spent_utxo, rows_to_clear);
    else if (table_name == "blocks"_n)
        clear_table(_block, rows_to_clear);
    else if (table_name == "block.extra"_n)
        _block_extra.remove();
    else if (table_name == "consensusblk"_n)
        clear_table(_consensus_block, rows_to_clear);
    else if (table_name == "chainstate"_n)
        _chain_state.remove();
    else if (table_name == "config"_n)
        _config.remove();
    else
        check(false, "utxomng.xsat::cleartable: [table_name] unknown table to clear");
}

[[eosio::action]]
std::vector<unsigned char> utxo_manage::addrtoscript(const string& btc_address) {
    std::vector<unsigned char> script;
    string error;
    if (!bitcoin::DecodeDestination(btc_address, script, error)) {
        check(false, error);
    }
    return script;
}

[[eosio::action]]
bool utxo_manage::isvalid(const string& btc_address) {
    return bitcoin::IsValid(btc_address);
}

[[eosio::action]]
void utxo_manage::setirrhash(const checksum256& irreversible_hash) {
    auto chain_state = _chain_state.get();
    chain_state.irreversible_hash = irreversible_hash;
    _chain_state.set(chain_state, get_self());
}