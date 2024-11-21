template <typename T>
void block_sync::clear_table(T &table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void block_sync::cleartable(const name table_name, const name &synchronizer, const uint64_t height,
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
        auto iter
            = eosio::internal_use_do_not_use::db_lowerbound_i64(get_self().value, bucket_id, BLOCK_CHUNK.value, 0);
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

[[eosio::action]]
void block_sync::reset(const name &synchronizer, const uint64_t height, const checksum256 &hash) {
    require_auth(get_self());

    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_idx = _block_bucket.get_index<"byblockid"_n>();
    auto block_bucket_itr = block_bucket_idx.require_find(xsat::utils::compute_block_id(height, hash));

    block_bucket_idx.modify(block_bucket_itr, same_payer, [&](auto &row) {
        row.verify_info = std::nullopt;
        row.status = upload_complete;
    });
}

[[eosio::action]]
void block_sync::updateparent(const name &synchronizer, const uint64_t height, const checksum256 &hash,
                              const checksum256 &parent) {
    require_auth(get_self());

    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_idx = _block_bucket.get_index<"byblockid"_n>();
    auto block_bucket_itr = block_bucket_idx.require_find(xsat::utils::compute_block_id(height, hash));

    block_bucket_idx.modify(block_bucket_itr, same_payer, [&](auto &row) {
        row.verify_info->previous_block_hash = parent;
    });
}

[[eosio::action]]
void block_sync::forkblock(const name &synchronizer, const uint64_t height, const checksum256 &hash,
                           const name &account, const uint32_t nonce) {
    require_auth(get_self());

    block_bucket_table _block_bucket = block_bucket_table(get_self(), synchronizer.value);
    auto block_bucket_idx = _block_bucket.get_index<"byblockid"_n>();
    auto block_bucket_itr = block_bucket_idx.require_find(xsat::utils::compute_block_id(height, hash));

    auto block_data = read_bucket(get_self(), block_bucket_itr->bucket_id, BLOCK_CHUNK, 0, block_bucket_itr->size);
    eosio::datastream<const char *> block_stream(block_data.data(), block_data.size());
    bitcoin::core::block_header block_header;
    block_stream >> block_header;

    auto num_transactions = bitcoin::varint::decode(block_stream);
    std::vector<bitcoin::core::transaction> transactions;
    transactions.reserve(num_transactions);
    for (auto i = 0; i < num_transactions; i++) {
        bitcoin::core::transaction transaction(&block_data, false);
        block_stream >> transaction;
        transactions.emplace_back(std::move(transaction));
    }

    auto script = xsat::utils::encode_op_return_eos_account(account);
    transactions[0].outputs[0].script.data = script;

    bool mutated;
    block_header.merkle = bitcoin::core::generate_header_merkle(transactions, &mutated);
    block_header.bits = bitcoin::compact::encode(std::numeric_limits<bitcoin::uint256_t>::max());
    block_header.nonce = nonce;

    bitcoin::uint256_t header_hash = block_header.hash();
    bitcoin::uint256_t target = block_header.target();

    eosio::print_f("max bits % ", block_header.bits);
    eosio::print_f("hash % target % ", bitcoin::be_checksum256_from_uint(header_hash),
                   bitcoin::be_checksum256_from_uint(target));
    check(header_hash <= target, "invalid_target");

    eosio::datastream<size_t> ps;
    ps << block_header;
    bitcoin::varint::encode(ps, transactions.size());
    for (const auto transaction : transactions) {
        ps << transaction;
    }
    auto serialized_size = ps.tellp();

    std::vector<char> hash_data;
    hash_data.resize(serialized_size);
    eosio::datastream<char *> ds(hash_data.data(), hash_data.size());
    ds << block_header;
    bitcoin::varint::encode(ds, transactions.size());
    for (const auto transaction : transactions) {
        ds << transaction;
    }
    eosio::printhex(hash_data.data(), hash_data.size());
}

[[eosio::action]]
uint32_t block_sync::getbits(uint32_t timestamp, const uint64_t p_height, const uint32_t p_timestamp,
                             const uint32_t p_bits) {
    auto parent_block = bitcoin::core::block{.height = p_height, .timestamp = p_timestamp, .bits = p_bits};
    return bitcoin::core::get_next_work_required(parent_block, timestamp, utxo_manage::get_ancestor, CHAIN_PARAMS);
}
