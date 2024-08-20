template <typename T>
void block_sync::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void block_sync::cleartable(const name table_name, const name& synchronizer, const uint64_t height,
                            const uint64_t bucket_id, const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;

    block_bucket_table _block_bucket(get_self(), synchronizer.value);
    passed_index_table _pass_index(get_self(), height);
    block_miner_table _block_miner(get_self(), height);

    if (table_name == "globalid"_n)
        _global_id.remove();
    else if (table_name == "blockbuckets"_n)
        clear_table(_block_bucket, rows_to_clear);
    else if (table_name == "block.chunk"_n) {
        auto iter = eosio::internal_use_do_not_use::db_find_i64(get_self().value, bucket_id, BLOCK_CHUNK.value, 0);
        while (iter >= 0) {
            uint64_t ignored;
            auto next_iter = eosio::internal_use_do_not_use::db_next_i64(iter, &ignored);
            eosio::internal_use_do_not_use::db_remove_i64(iter);
            iter = next_iter;
        }
    } else if (table_name == "passedindexs"_n)
        clear_table(_pass_index, rows_to_clear);
    else if (table_name == "blockminer"_n)
        clear_table(_block_miner, rows_to_clear);
    else
        check(false, "blksync.xsat::cleartable: [table_name] unknown table to clear");
}
