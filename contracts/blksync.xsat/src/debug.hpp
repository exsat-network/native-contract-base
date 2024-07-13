template <typename T>
void block_sync::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void block_sync::cleartable(const name table_name, const name& synchronizer, const uint64_t height,
                            const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;

    block_bucket_table _block_bucket(get_self(), synchronizer.value);
    block_chunk_table _block_chunk(get_self(), height);
    passed_index_table _pass_index(get_self(), height);

    if (table_name == "globalid"_n)
        _global_id.remove();
    else if (table_name == "blockbuckets"_n)
        clear_table(_block_bucket, rows_to_clear);
    else if (table_name == "block.chunk"_n)
        clear_table(_block_chunk, rows_to_clear);
    else if (table_name == "passedindexs"_n)
        clear_table(_pass_index, rows_to_clear);
    else
        check(false, "blksync.xsat::cleartable: [table_name] unknown table to clear");
}
