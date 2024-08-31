#pragma once

#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <sstream>
#include <utxomng.xsat/utxomng.xsat.hpp>
#include <endrmng.xsat/endrmng.xsat.hpp>
#include <bitcoin/utility/address_converter.hpp>
#include "../internal/defines.hpp"
#include "../internal/utils.hpp"
#include "../internal/safemath.hpp"

using namespace eosio;
using namespace std;

class [[eosio::contract("custody.xsat")]] custody : public contract {

public:
    using contract::contract;

    /**
     * ## ACTION `addcustody`
     *
     * - **authority**: `get_self()`
     *
     * > Add custody record
     *
     * ### params
     *
     * - `{checksum160} staker` - staker evm address
     * - `{checksum160} proxy` - proxy evm address
     * - `{name} validator` - validator account
     * - `{optional<string>} btc_address` - bitcoin address
     * - `{optional<vector<uint8_t>>} scriptpubkey` - scriptpubkey
     *
     * ### example
     *
     * ```bash
     * $ cleos push action custody.xsat addcustody '["1231deb6f5749ef6ce6943a275a1d3e7486f4eae", "0000000000000000000000000000000000000001", "val1.xsat", "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN", null]' -p custody.xsat
     * ```
     */
    [[eosio::action]]
    void addcustody(const checksum160 staker, const checksum160 proxy, const name validator, const optional<string> btc_address, optional<vector<uint8_t>> scriptpubkey);

    /**
     * ## ACTION `updcustody`
     *
     * - **authority**: `get_self()`
     *
     * > Update custody record
     *
     * ### params
     *
     * - `{checksum160} staker` - staker evm address
     * - `{name} validator` - validator account
     *
     * ### example
     *
     * ```bash
     * $ cleos push action custody.xsat updcustody '["1231deb6f5749ef6ce6943a275a1d3e7486f4eae", "val1.xsat"]' -p custody.xsat
     * ```
     */
    [[eosio::action]]
    void updcustody(const checksum160 staker, const name validator);

    /**
     * ## ACTION `delcustody`
     *
     * - **authority**: `get_self()`
     *
     * > Delete custody record
     *
     * ### params
     *
     * - `{checksum160} staker` - staker evm address
     *
     * ### example
     *
     * ```bash
     * $ cleos push action custody.xsat delcustody '["1231deb6f5749ef6ce6943a275a1d3e7486f4eae"]' -p custody.xsat
     * ```
     */
    [[eosio::action]]
    void delcustody(const checksum160 staker);

    /**
     * ## ACTION `syncstake`
     *
     * - **authority**: `anyone`
     *
     * > Sync staker btc address stake
     *
     * ### example
     *
     * ```bash
     * $ cleos push action custody.xsat syncstake '[]' -p custody.xsat
     * ```
     */
    [[eosio::action]]
    void syncstake(optional<uint64_t> process_rows);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
    [[eosio::action]]
    void addr2pubkey(const string& address);
    [[eosio::action]]
    void pubkey2addr(const vector<uint8_t> data);
    [[eosio::action]]
    void updateheight(const uint64_t height);
#endif

private:
    /**
     * ## TABLE `globalid`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} custody_id` - the latest custody id
     *
     * ### example
     *
     * ```json
     * {
     *   "custody_id": 1
     * }
     * ```
     */
    struct [[eosio::table]] global_id_row {
        uint64_t custody_id;
        uint64_t last_custody_id;
        uint64_t last_height;
    };
    typedef singleton<"globalid"_n, global_id_row> global_id_table;
    global_id_table _global_id = global_id_table(_self, _self.value);

    /**
     * ## TABLE `custodies`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} id` - the custody id
     * - `{checksum160} staker` - the staker evm address
     * - `{checksum160} proxy` - the proxy evm address
     * - `{name} validator` - the validator account
     * - `{string} btc_address` - the bitcoin address
     * - `{vector<uint8_t>} scriptpubkey` - the scriptpubkey
     * - `{uint64_t} value` - the total utxo value
     * - `{time_point_sec} latest_stake_time` - the latest stake time
     *
     * ### example
     *
     * ```
     * {
     *   "id": 7,
     *   "staker": "ee37064f01ec9314278f4984ff4b9b695eb91912",
     *   "proxy": "0000000000000000000000000000000000000001",
     *   "validator": "val1.xsat",
     *   "btc_address": "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN",
     *   "scriptpubkey": "a914cac3a79a829c31b07e6a8450c4e05c4289ab95b887"
     *   "value": 100000000,
     *   "latest_stake_time": "2021-09-01T00:00:00"
     * }
     * ```
     *
     */
    struct [[eosio::table]] custody_row {
        uint64_t id;
        checksum160 staker;
        checksum160 proxy;
        name validator;
        string btc_address;
        std::vector<uint8_t> scriptpubkey;
        uint64_t value;
        time_point_sec latest_stake_time;
        uint64_t primary_key() const { return id; }
        checksum256 by_staker() const { return xsat::utils::compute_id(staker); }
        checksum256 by_scriptpubkey() const { return xsat::utils::hash(scriptpubkey); }
    };
    typedef eosio::multi_index<"custodies"_n, custody_row,
        eosio::indexed_by<"bystaker"_n, const_mem_fun<custody_row, checksum256, &custody_row::by_staker>>,
        eosio::indexed_by<"scriptpubkey"_n, const_mem_fun<custody_row, checksum256, &custody_row::by_scriptpubkey>>>
        custody_index;

    uint64_t next_custody_id();
    uint64_t current_irreversible_height();

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};