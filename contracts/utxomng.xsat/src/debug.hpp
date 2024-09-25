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

    if (table_name == "utxos"_n)
        clear_table(_utxo, rows_to_clear);
    else if (table_name == "pendingutxos"_n)
        clear_table(_pending_utxo, rows_to_clear);
    else if (table_name == "spentutxos"_n)
        clear_table(_spent_utxo, rows_to_clear);
    else if (table_name == "blocks"_n)
        clear_table(_block, rows_to_clear);
    else if (table_name == "block.extra"_n)
        clear_table(_block_extra, rows_to_clear);
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
std::vector<string> utxo_manage::scripttoaddr(const vector<uint8_t>& scriptpubkey) {
    require_auth(get_self());

    std::vector<string> scriptpubkey_addresses;
    bitcoin::ExtractDestination(scriptpubkey, CHAIN_PARAMS, scriptpubkey_addresses);
    return scriptpubkey_addresses;
}

[[eosio::action]]
std::vector<unsigned char> utxo_manage::addrtoscript(const string& btc_address) {
    require_auth(get_self());
    std::vector<unsigned char> script;
    string error;
    if (!bitcoin::DecodeDestination(btc_address, script, CHAIN_PARAMS, error)) {
        check(false, error);
    }
    return script;
}

[[eosio::action]]
bool utxo_manage::isvalid(const string& btc_address) {
    require_auth(get_self());
    return bitcoin::IsValid(btc_address, CHAIN_PARAMS);
}

[[eosio::action]]
void utxo_manage::setirrhash(const checksum256& irreversible_hash) {
    require_auth(get_self());
    auto chain_state = _chain_state.get();
    chain_state.irreversible_hash = irreversible_hash;
    _chain_state.set(chain_state, get_self());
}

[[eosio::action]]
bool utxo_manage::unspendable(const uint64_t height, const vector<uint8_t>& script) {
    require_auth(get_self());
    return xsat::utils::is_unspendable(height, script);
}

[[eosio::action]]
void utxo_manage::resetpending(uint64_t row) {
    require_auth(get_self());

    auto pending_utxo_itr = _pending_utxo.begin();
    auto pending_utxo_end = _pending_utxo.end();
    while (pending_utxo_itr != pending_utxo_end && row--) {
        pending_utxo_itr = _pending_utxo.erase(pending_utxo_itr);
    }
    if (pending_utxo_itr == pending_utxo_end) {
        auto chain_state = _chain_state.get();
        auto config = _config.get();
        auto consensus_block_idx = _consensus_block.get_index<"byheight"_n>();
        auto consensus_block_itr = consensus_block_idx.lower_bound(chain_state.irreversible_height + 1);
        auto consensus_end = consensus_block_idx.upper_bound(chain_state.irreversible_height + 1);

        chain_state.parsed_height = chain_state.irreversible_height;
        chain_state.parsing_height = chain_state.irreversible_height + 1;
        while (consensus_block_itr != consensus_end) {
            chain_state.parsing_progress_of[consensus_block_itr->hash]
                = {.bucket_id = consensus_block_itr->bucket_id,
                   .parser = consensus_block_itr->synchronizer,
                   .parse_expiration_time = current_time_point() + eosio::seconds(config.parse_timeout_seconds)};
            consensus_block_itr++;
        }
        chain_state.status = waiting;
        _chain_state.set(chain_state, get_self());
    }
}

[[eosio::action]]
void utxo_manage::addtestblock(const uint64_t height, const checksum256& hash, const checksum256& cumulative_work,
                               const uint32_t version, const checksum256& previous_block_hash,
                               const checksum256& merkle, const uint32_t timestamp, const uint32_t bits,
                               const uint32_t nonce) {
    require_auth(get_self());

    auto block_itr = _block.find(height);
    if (block_itr == _block.end()) {
        _block.emplace(get_self(), [&](auto& row) {
            row.height = height;
            row.hash = hash;
            row.cumulative_work = cumulative_work;
            row.version = version;
            row.previous_block_hash = previous_block_hash;
            row.merkle = merkle;
            row.timestamp = timestamp;
            row.bits = bits;
            row.nonce = nonce;
        });
    } else {
        _block.modify(block_itr, same_payer, [&](auto& row) {
            row.height = height;
            row.hash = hash;
            row.cumulative_work = cumulative_work;
            row.version = version;
            row.previous_block_hash = previous_block_hash;
            row.merkle = merkle;
            row.timestamp = timestamp;
            row.bits = bits;
            row.nonce = nonce;
        });
    }
}