#warning "building debug actions for rwddist.xsat"

template <typename T>
void reward_distribution::clear_table(T& table, uint64_t rows_to_clear) {
    auto itr = table.begin();
    while (itr != table.end() && rows_to_clear--) {
        itr = table.erase(itr);
    }
}

[[eosio::action]]
void reward_distribution::cleartable(const name table_name, const optional<name> scope,
                                     const optional<uint64_t> max_rows) {
    require_auth(get_self());
    const uint64_t rows_to_clear = (!max_rows || *max_rows == 0) ? -1 : *max_rows;
    const uint64_t value = scope ? scope->value : get_self().value;

    reward_balance_table _reward_balance(get_self(), value);
    reward_log_table _reward_log(get_self(), value);

    // tables
    if (table_name == "rewardlogs"_n)
        clear_table(_reward_log, rows_to_clear);
    else if (table_name == "rewardbal"_n)
        _reward_balance.remove();
    else
        check(false, "rwddist.xsat::cleartable: [table_name] unknown table to clear");
}

[[eosio::action]]
void reward_distribution::setstate(uint64_t height, name miner, name synchronizer, std::vector<name> provider_validators, uint64_t staking) {
    require_auth(get_self());

    utxo_manage::chain_state_table _chain_state(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);

    auto chain_state = _chain_state.get();
    chain_state.migrating_height = height;
    chain_state.migrating_hash = checksum256();
    *(uint64_t *)&(chain_state.migrating_hash) = height;
    chain_state.miner = miner;
    chain_state.synchronizer = synchronizer;
    chain_state.parser = synchronizer;

    if (get_self() == UTXO_MANAGE_CONTRACT) {
        _chain_state.set(chain_state, get_self());
    }

    block_endorse::endorsement_table _endorsement(BLOCK_ENDORSE_CONTRACT, height);

    if (get_self() == BLOCK_ENDORSE_CONTRACT) {
        _endorsement.emplace(get_self(), [&](block_endorse::endorsement_row &row) {
            row.id = _endorsement.available_primary_key();
            row.hash = chain_state.migrating_hash;
            row.provider_validators.resize(provider_validators.size());
            // for (size_t i = 0; i < num_validators; ++i) {
            //     row.provider_validators[i].account = provider_validators[i];
            //     row.provider_validators[i].staking = staking;
            //     row.provider_validators[i].created_at = time_point_sec();
            // }
        });
    }
}
