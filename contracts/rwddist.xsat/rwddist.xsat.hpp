#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "../internal/defines.hpp"
#include <endrmng.xsat/endrmng.xsat.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("rwddist.xsat")]] reward_distribution : public contract {
   public:
    using contract::contract;

    /**
     * ## STRUCT `validator_info`
     *
     * - `{name} account` - validator account
     * - `{uint64_t} staking` - the number of validator pledges
     *
     * ### example
     *
     * ```json
     * {
     *   "account": "test.xsat",
     *   "staking": "10200000000"
     * }
     * ```
     */
    struct validator_info {
        name account;
        uint64_t staking;
    };

    /**
     * ## TABLE `rewardlogs`
     *
     * ### scope `height`
     * ### params
     *
     * - `{uint64_t} height` - block height
     * - `{checksum256} hash` - block hash
     * - `{asset} synchronizer_rewards` - the synchronizer assigns the number of rewards
     * - `{asset} consensus_rewards` - the consensus validator allocates the number of rewards
     * - `{asset} staking_rewards` - the validator assigns the number of rewards
     * - `{uint32_t} num_validators` - the number of validators who pledge more than 100 BTC
     * - `{std::vector<validator_info> } provider_validators` - list of endorsed validators
     * - `{uint64_t} endorsed_staking` - total endorsed pledge amount
     * - `{uint64_t} reached_consensus_staking` - the total pledge amount to reach consensus is
     * `(number of validators * 2/3+ 1 pledge amount)`
     * - `{uint32_t} num_validators_assigned` - the number of validators that have been allocated rewards
     * - `{name} synchronizer` -synchronizer account
     * - `{name} miner` - miner account
     * - `{name} parser` - parse the account of the block
     * - `{checksum256} tx_id` - tx_id of the reward after distribution
     * - `{time_point_sec} latest_exec_time` - latest reward distribution time
     *
     * ### example
     *
     * ```json
     * {
     *   "height": 840000,
     *   "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5",
     *   "synchronizer_rewards": "5.00000000 XSAT",
     *   "consensus_rewards": "5.00000000 XSAT",
     *   "staking_rewards": "40.00000000 XSAT",
     *   "num_validators": 2,
     *   "provider_validators": [{
     *       "account": "alice",
     *       "staking": "10010000000"
     *       },{
     *       "account": "bob",
     *       "staking": "10200000000"
     *       }
     *   ],
     *   "endorsed_staking": "20210000000",
     *   "reached_consensus_staking": "20210000000",
     *   "num_validators_assigned": 2,
     *   "synchronizer": "alice",
     *   "miner": "",
     *   "parser": "alice",
     *   "tx_id": "0000000000000000000000000000000000000000000000000000000000000000",
     *   "latest_exec_time": "2024-07-13T09:06:56"
     * }
     * ```
     */
    struct [[eosio::table]] reward_log_row {
        uint64_t height;
        checksum256 hash;
        asset synchronizer_rewards;
        asset consensus_rewards;
        asset staking_rewards;
        uint32_t num_validators;
        std::vector<validator_info> provider_validators;
        uint64_t endorsed_staking;
        uint64_t reached_consensus_staking;
        uint32_t num_validators_assigned;
        name synchronizer;
        name miner;
        name parser;
        checksum256 tx_id;
        time_point_sec latest_exec_time;
        uint64_t primary_key() const { return height; }
    };
    typedef eosio::multi_index<"rewardlogs"_n, reward_log_row> reward_log_table;

    /**
     * ## ACTION `distribute`
     *
     * - **authority**: `utxomng.xsat`
     *
     * > Allocate rewards and record allocation information.
     *
     * ### params
     *
     * - `{uint64_t} height` - Block height for allocating rewards
     *
     * ### example
     *
     * ```bash
     * $ cleos push action rwddist.xsat distribute '[840000]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void distribute(const uint64_t height);

    /**
     * ## ACTION `endtreward`
     *
     * - **authority**: `utxomng.xsat`
     *
     * > Allocate rewards and record allocation information.
     *
     * ### params
     *
     * - `{name} parser` - parse account
     * - `{uint64_t} height` - block height
     * - `{uint32_t} from_index` - the starting reward index of provider_validators
     * - `{uint32_t} to_index` - end reward index of provider_validators
     *
     * ### example
     *
     * ```bash
     * $ cleos push action rwddist.xsat endtreward '["alice", 840000, 0, 10]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void endtreward(const name& parser, const uint64_t height, uint32_t from_index, const uint32_t to_index);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
#endif

    // logs
    [[eosio::action]]
    void rewardlog(const uint64_t height, const checksum256& hash, const name& parser, const asset& synchronizer_reward,
                   const asset& staking_reward, const asset& consensus_reward) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void endtrwdlog(const uint64_t height, const checksum256& hash,
                    const vector<endorse_manage::reward_details_row>& reward_details) {
        require_auth(get_self());
    }

    using distribute_action = eosio::action_wrapper<"distribute"_n, &reward_distribution::distribute>;
    using endtreward_action = eosio::action_wrapper<"endtreward"_n, &reward_distribution::endtreward>;
    using rewardlog_action = eosio::action_wrapper<"rewardlog"_n, &reward_distribution::rewardlog>;
    using endtrwdlog_action = eosio::action_wrapper<"endtrwdlog"_n, &reward_distribution::endtrwdlog>;

   private:
    // init table
    reward_log_table _reward_log = reward_log_table(_self, _self.value);

    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};