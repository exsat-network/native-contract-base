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
     * @brief synchronizer table.
     * @scope `get_self()`
     *
     * @field synchronizer - synchronizer address
     * @field reward_recipient - get rewards to address
     * @field memo - memo when receiving rewards
     * @field num_slots - number of slots owned
     * @field latest_produced_block_height - the height of the latest mined block
     * @field produced_block_limit - upload limit, for example, the number of blocks in 432 can only be uploaded. 0 is
     * not limited
     * @field unclaimed - amount of unclaimed rewards
     * @field claimed  - amount of claimed rewards
     * @field latest_reward_block  - amount of claimed rewards
     * @field latest_reward_time  - amount of claimed rewards
     *
     */
    struct [[eosio::table]] synchronizer_row {
        name synchronizer;
        name reward_recipient;
        string memo;
        uint16_t num_slots;
        uint64_t latest_produced_block_height;
        uint16_t produced_block_limit;
        asset unclaimed;
        asset claimed;
        uint64_t latest_reward_block;
        time_point_sec latest_reward_time;
        uint64_t primary_key() const { return synchronizer.value; }
    };
    typedef eosio::multi_index<"synchronizer"_n, synchronizer_row> synchronizer_table;

    /**
     * @brief miner table.
     * @scope `get_self()`
     *
     * @field id - primary key
     * @field synchronizer - synchronizer address
     * @field miner - miner address
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
     * @brief Update synchronizer produced block height action.
     * @auth  `blksync.xsat`
     *
     * @param synchronizer - synchronizer account
     * @param latest_produced_block_height - the height of the latest mined block
     * @param miners - list of btc addresses corresponding to miner
     *
     */
    [[eosio::action]]
    void updateheight(const name& synchronizer, const uint64_t latest_produced_block_height,
                      const std::vector<string>& miners);

    /**
     * @brief Unbundle action.
     * @auth  `get_self()`
     *
     * @param id - miner id
     *
     */
    [[eosio::action]]
    void unbundle(const uint64_t id);

    /**
     * @brief init pool action.
     * @auth `get_self()`
     *
     * @param synchronizer - synchronizer account
     * @param latest_produced_block_height - the height of the latest mined block
     * @param miners - list of btc addresses corresponding to miner
     * @param financial_account - financial account
     *
     */
    [[eosio::action]]
    void initpool(const name& synchronizer, const uint64_t latest_produced_block_height,
                  const string& financial_account, const std::vector<string>& miners);

    /**
     * @brief config synchronizer produced_block_limit action.
     * @auth `get_self()`
     *
     * @param synchronizer - synchronizer account
     * @param produced_block_limit - upload limit, for example, the number of blocks in 432 can only be uploaded. 0
     * is
     *
     */
    [[eosio::action]]
    void config(const name& synchronizer, const uint16_t produced_block_limit);

    /**
     * @brief add slot action.
     * @auth `synchronizer`
     *
     * @param synchronizer - synchronizer account
     * @param receiver - the synchronizer account of the receiving slot
     * @param num_slots - number of slots
     *
     */
    [[eosio::action]]
    void buyslot(const name& synchronizer, const name& receiver, const uint16_t num_slots);

    /**
     * @brief add slot action.
     * @auth `synchronizer`
     *
     * @param synchronizer - synchronizer account
     * @param financial_account - financial account
     *
     */
    [[eosio::action]]
    void setfinacct(const name& synchronizer, const string& financial_account);

    /**
     * @brief add slot action.
     * @auth synchronizer->to or evmutil.xsat
     *
     * @param synchronizer - synchronizer account
     *
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
    void claimlog(const name& synchronizer, const string& reward_recipient, const asset& quantity) {
        require_auth(get_self());
    }

    using updateheight_action = eosio::action_wrapper<"updateheight"_n, &pool::updateheight>;
    using buyslot_action = eosio::action_wrapper<"buyslot"_n, &pool::buyslot>;
    using claimlog_action = eosio::action_wrapper<"claimlog"_n, &pool::claimlog>;
    using poollog_action = eosio::action_wrapper<"poollog"_n, &pool::poollog>;

   private:
    synchronizer_table _synchronizer = synchronizer_table(_self, _self.value);
    miner_table _miner = miner_table(_self, _self.value);

    void save_miners(const name& synchronizer, const vector<string>& miners);
    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};