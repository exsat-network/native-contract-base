#pragma once

#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include "../internal/utils.hpp"

using namespace eosio;
using namespace std;

class [[eosio::contract("poolreg.xsat")]] pool : public contract {
   public:
    using contract::contract;

    /**
     * ## TABLE `config`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{string} donation_account` - the account designated for receiving donations
     * 
     * ### example
     *
     * ```json
     * {
     *   "donation_account": "donate.xsat"
     * }
     * ```
     */
    struct [[eosio::table]] config_row {
        string donation_account;
    };
    typedef eosio::singleton<"config"_n, config_row> config_table;

    /**
     * ## TABLE `synchronizer`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{name} reward_recipient` - receiving account for receiving rewards
     * - `{string} memo` - memo when receiving reward transfer
     * - `{uint16_t} num_slots` - number of slots owned
     * - `{uint64_t} latest_produced_block_height` - the latest block number
     * - `{uint16_t} produced_block_limit` - upload block limit, for example, if 432 is set, the upload height needs to be a synchronizer that has produced blocks in 432 blocks before it can be uploaded.
     * - `{uint16_t} donate_rate` - the donation rate, represented as a percentage, ex: 500 means 5.00%
     * - `{asset} total_donated` - the total amount of XSAT that has been donated
     * - `{asset} unclaimed` - unclaimed rewards
     * - `{asset} claimed` - rewards claimed
     * - `{uint64_t} latest_reward_block` - the latest block number to receive rewards
     * - `{time_point_sec} latest_reward_time` - latest reward time
     *
     * ### example
     *
     * ```json
     * {
     *    "synchronizer": "test.xsat",
     *    "reward_recipient": "erc2o.xsat",
     *    "memo": "0x4838b106fce9647bdf1e7877bf73ce8b0bad5f97",
     *    "num_slots": 2,
     *    "latest_produced_block_height": 840000,
     *    "produced_block_limit": 432,
     *    "donate_rate": 100,
     *    "total_donated": "100.00000000 XSAT",
     *    "unclaimed": "5.00000000 XSAT",
     *    "claimed": "0.00000000 XSAT",
     *    "latest_reward_block": 840001,
     *    "latest_reward_time": "2024-07-13T14:29:32"
     * }
     * ```
     */
    struct [[eosio::table]] synchronizer_row {
        name synchronizer;
        name reward_recipient;
        string memo;
        uint16_t num_slots;
        uint64_t latest_produced_block_height;
        uint16_t produced_block_limit;
        uint16_t donate_rate;
        asset total_donated;
        asset unclaimed;
        asset claimed;
        uint64_t latest_reward_block;
        time_point_sec latest_reward_time;
        uint64_t primary_key() const { return synchronizer.value; }
        uint64_t by_total_donated() const { return total_donated.amount; }
    };
    typedef eosio::multi_index<
        "synchronizer"_n, synchronizer_row,
        eosio::indexed_by<"bydonate"_n, const_mem_fun<synchronizer_row, uint64_t, &synchronizer_row::by_total_donated>>>
        synchronizer_table;

    /**
     * ## TABLE `miners`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} id` - primary key
     * - `{name} synchronizer` - synchronizer account
     * - `{string} miner` - associated btc miner account
     *
     * ### example
     *
     * ```json
     * {
     *    "id": 1,
     *    "synchronizer": "alice",
     *    "miner": "3PiyiAezRdSUQub3ewUXsgw5M6mv6tskGv"
     * }
     * ```
     *
     */
    struct [[eosio::table]] miner_row {
        uint64_t id;
        name synchronizer;
        std::string miner;
        uint64_t primary_key() const { return id; }
        uint64_t by_syncer() const { return synchronizer.value; }
        checksum256 by_miner() const { return xsat::utils::hash(miner); }
    };
    typedef eosio::multi_index<
        "miners"_n, miner_row,
        eosio::indexed_by<"bysyncer"_n, const_mem_fun<miner_row, uint64_t, &miner_row::by_syncer>>,
        eosio::indexed_by<"byminer"_n, const_mem_fun<miner_row, checksum256, &miner_row::by_miner>>>
        miner_table;

    /**
     * ## TABLE `stat`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{asset} xsat_total_donated` - the cumulative amount of XSAT donated
     *
     * ### example
     *
     * ```json
     * {
     *   "xsat_total_donated": "100.40000000 XSAT"
     * }
     * ```
     */
    struct [[eosio::table]] stat_row {
        asset xsat_total_donated = {0, XSAT_SYMBOL};
    };
    typedef eosio::singleton<"stat"_n, stat_row> stat_table;

    /**
     * ## ACTION `setdonateacc`
     *
     * - **authority**: `get_self()`
     *
     * > Update donation account.
     *
     * ### params
     *
     * - `{string} donation_account` -  account to receive donations
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat setdonateacc '["alice"]' -p poolreg.xsat
     * ```
     */
    void setdonateacc(const string& donation_account);

    /**
     * ## ACTION `updateheight`
     *
     * - **authority**: `utxomng.xsat`
     *
     * > Update synchronizer’s latest block height and add associated btc miners.
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} latest_produced_block_height` - the height of the latest mined block
     * - `{std::vector<string>} miners` - list of btc accounts corresponding to synchronizer
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat updateheight '["alice", 839999, ["3PiyiAezRdSUQub3ewUXsgw5M6mv6tskGv", "bc1p8k4v4xuz55dv49svzjg43qjxq2whur7ync9tm0xgl5t4wjl9ca9snxgmlt"]]' -p utxomng.xsat
     * ```
     */
    [[eosio::action]]
    void updateheight(const name& synchronizer, const uint64_t latest_produced_block_height,
                      const std::vector<string>& miners);

    /**
     * ## ACTION `initpool`
     *
     * - **authority**: `get_self()`
     *
     * > Unbind the association between synchronizer and btc miner.
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint64_t} latest_produced_block_height` - the height of the latest mined block
     * - `{string} financial_account` - financial account to receive rewards
     * - `{std::vector<string>} miners` - list of btc accounts corresponding to synchronizer
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat initpool '["alice", 839997, "alice", ["37jKPSmbEGwgfacCr2nayn1wTaqMAbA94Z", "39C7fxSzEACPjM78Z7xdPxhf7mKxJwvfMJ"]]' -p poolreg.xsat
     * ```
     */
    [[eosio::action]]
    void initpool(const name& synchronizer, const uint64_t latest_produced_block_height,
                  const string& financial_account, const std::vector<string>& miners);

    /**
     * ## ACTION `delpool`
     *
     * - **authority**: `get_self()`
     *
     * > Erase synchronizer.
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat delpool '["alice"]' -p poolreg.xsat
     * ```
     */
    [[eosio::action]]
    void delpool(const name& synchronizer);

    /**
     * ## ACTION `unbundle`
     *
     * - **authority**: `get_self()`
     *
     * > Unbind the association between synchronizer and btc miner.
     *
     * ### params
     *
     * - `{uint64_t} id` - primary key of miners table
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat unbundle '[1]' -p poolreg.xsat
     * ```
     */
    [[eosio::action]]
    void unbundle(const uint64_t id);

    /**
     * ## ACTION `config`
     *
     * - **authority**: `get_self()`
     *
     * > Configure synchronizer block output limit.
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint16_t} produced_block_limit` - upload block limit, for example, if 432 is set, the upload height needs to be a synchronizer that has produced blocks in 432 blocks before it can be uploaded.
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat config '["alice", 432]' -p poolreg.xsat
     * ```
     */
    [[eosio::action]]
    void config(const name& synchronizer, const uint16_t produced_block_limit);

    /**
     * ## ACTION `setdonate`
     *
     * - **authority**: `synchronizer`
     *
     * > Configure donate rate.
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{uint16_t} donate_rate` - the donation rate, represented as a percentage, ex: 500 means 5.00% 
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat setdonate '["alice", 100]' -p alice
     * ```
     */
    [[eosio::action]]
    void setdonate(const name& synchronizer, const uint16_t donate_rate);

    /**
     * ## ACTION `buyslot`
     *
     * - **authority**: `synchronizer`
     *
     * > Buy slot.
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{name} receiver` - the account of the receiving slot
     * - `{uint16_t} num_slots` - number of slots
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat buyslot '["alice", "alice", 2]' -p alice
     * ```
     */
    [[eosio::action]]
    void buyslot(const name& synchronizer, const name& receiver, const uint16_t num_slots);

    /**
     * ## ACTION `setfinacct`
     *
     * - **authority**: `synchronizer`
     *
     * > Configure financial account.
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     * - `{string} financial_account` - financial account to receive rewards
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat setfinacct '["alice", "alice"]' -p alice
     * ```
     */
    [[eosio::action]]
    void setfinacct(const name& synchronizer, const string& financial_account);

    /**
     * ## ACTION `claim`
     *
     * - **authority**: `synchronizer->to or evmutil.xsat`
     *
     * > Receive award.
     *
     * ### params
     *
     * - `{name} synchronizer` - synchronizer account
     *
     * ### example
     *
     * ```bash
     * $ cleos push action poolreg.xsat claim '["alice"]' -p alice
     * ```
     */
    [[eosio::action]]
    void claim(const name& synchronizer);

    [[eosio::on_notify("*::transfer")]]
    void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
#endif

    // logs
    [[eosio::action]]
    void poollog(const name& synchronizer, const uint64_t latest_produced_block_height,
                 const string& reward_recipient) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void delpoollog(const name& synchronizer) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void claimlog(const name& synchronizer, const string& reward_recipient, const asset& quantity,
                  const asset& donated_amount, const asset& total_donated) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void setdonatelog(const name& synchronizer, const uint16_t donate_rate) {
        require_auth(get_self());
    }

    using updateheight_action = eosio::action_wrapper<"updateheight"_n, &pool::updateheight>;
    using buyslot_action = eosio::action_wrapper<"buyslot"_n, &pool::buyslot>;
    using claimlog_action = eosio::action_wrapper<"claimlog"_n, &pool::claimlog>;
    using poollog_action = eosio::action_wrapper<"poollog"_n, &pool::poollog>;
    using delpoollog_action = eosio::action_wrapper<"delpoollog"_n, &pool::delpoollog>;
    using setdonatelog_action = eosio::action_wrapper<"setdonatelog"_n, &pool::setdonatelog>;

   private:
    synchronizer_table _synchronizer = synchronizer_table(_self, _self.value);
    miner_table _miner = miner_table(_self, _self.value);
    config_table _config = config_table(_self, _self.value);
    stat_table _stat = stat_table(_self, _self.value);

    void save_miners(const name& synchronizer, const vector<string>& miners);

    void token_transfer(const name& from, const string& to, const extended_asset& value);

    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};