#pragma once

#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <algorithm>
#include <sstream>
#include <vector>
#include <utxomng.xsat/utxomng.xsat.hpp>
#include <btc.xsat/btc.xsat.hpp>
#include "../internal/defines.hpp"
#include "../internal/utils.hpp"
#include "../internal/safemath.hpp"


using namespace eosio;
using namespace std;
using std::vector;

class [[eosio::contract("brdgmng.xsat")]] brdgmng : public contract {

public:
    using contract::contract;

    typedef uint8_t address_status;
    static const address_status address_status_initialized = 0;
    static const address_status address_status_confirmed = 1;

    typedef uint8_t deposit_status;
    static const deposit_status deposit_status_initialized = 0;
    static const deposit_status deposit_status_confirmed = 1;
    static const deposit_status deposit_status_failed = 3;
    static const deposit_status deposit_status_error = 4;

    static const uint64_t BTC_BASE = 100000000;

    [[eosio::action]]
    void initialize();

    // [[eosio::action]]
    // void updateconfig(const uint8_t status);

    [[eosio::action]]
    void addperm(const uint64_t id, const vector<name> names);

    [[eosio::action]]
    void delperm(const uint64_t id);

    [[eosio::action]]
    void addaddresses(const name actor, const uint64_t permission_id, string b_id, string wallet_code, const vector<string> btc_addresses);

    [[eosio::action]]
    void valaddress(const name actor, const uint64_t permission_id, const string btc_addresses);

    [[eosio::action]]
    void mappingaddr(const name actor, const uint64_t permission_id, const checksum160 evm_address);

    [[eosio::action]]
    void deposit(const name actor, uint64_t permission_id, string b_id, string wallet_code, string btc_address, string order_no,
        uint64_t block_height, string tx_id, uint64_t amount, string tx_status, string remark_detail, uint64_t tx_time_stamp, uint64_t create_time_stamp);

    [[eosio::action]]
    void valdeposit(const name actor, const uint64_t permission_id, const uint64_t deposit_id);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
#endif

private:
    struct [[eosio::table]] boot_row {
        bool initialized;
        bool returned;
        asset quantity;
    };
    typedef singleton<"boot"_n, boot_row> boot_table;
    boot_table _boot = boot_table(_self, _self.value);

    /**
     * ## TABLE `globalid`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} brdgmng_id` - the latest brdgmng id
     *
     * ### example
     *
     * ```json
     * {
     *   "block_sync_status": 0,
     *   "brdgmng_id": 1,
     *   "last_brdgmng_id": 1,
     *   "last_height": 0,
     *   "vault_id": 1
     * }
     * ```
     */
    struct [[eosio::table]] global_id_row {
        uint64_t permission_id;
        uint64_t address_id;
        uint64_t address_mapping_id;
        uint64_t deposit_id;
    };
    typedef singleton<"globalid"_n, global_id_row> global_id_table;
    global_id_table _global_id = global_id_table(_self, _self.value);

    struct [[eosio::table]] config_row {
        bool deposit_status;
        bool withdraw_status;
        bool check_uxto_status;
        uint64_t deposit_fee;
        uint64_t withdraw_fee;
        uint16_t btc_address_inactive_clear_days;
    };
    typedef singleton<"configs"_n, config_row> config_table;
    config_table _config = config_table(_self, _self.value);

    struct [[eosio::table]] statistics_row {
        uint64_t total_btc_address_count;
        uint64_t mapped_address_count;
        uint64_t total_deposit_amount;
        uint64_t total_withdraw_amount;
    };
    typedef singleton<"statistics"_n, statistics_row> statistics_table;
    statistics_table _statistics = statistics_table(_self, _self.value);

    struct [[eosio::table]] permission_row {
        uint64_t id; //不同的权限组id，就是不同的来源
        std::vector<name> actors;
        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index<"permissions"_n, permission_row> permission_index;

    struct [[eosio::table]] address_row {
        uint64_t id;
        uint64_t permission_id;
        std::vector<name> provider_actors;
        string b_id; //cactus业务线id
        string wallet_code; //cactus钱包编号
        string btc_address;
        address_status status;
        uint64_t primary_key() const { return id; }
        uint64_t by_permission_id() const { return permission_id; }
        checksum256 by_btc_addrress() const { return xsat::utils::hash(btc_address); }
    };
    typedef eosio::multi_index<"addresses"_n, address_row,
        eosio::indexed_by<"bypermission"_n, const_mem_fun<address_row, uint64_t, &address_row::by_permission_id>>,
        eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<address_row, checksum256, &address_row::by_btc_addrress>>>
        address_index;

    struct [[eosio::table]] address_mapping_row {
        uint64_t id;
        string b_id; //cactus业务线id
        string wallet_code; //cactus钱包编号
        string btc_address;
        checksum160 evm_address;
        time_point_sec last_bridge_time;
        uint64_t primary_key() const { return id; }
        checksum256 by_btc_address() const { return xsat::utils::hash(btc_address); }
        checksum256 by_evm_address() const { return xsat::utils::compute_id(evm_address); }
        uint64_t by_last_bridge_time() const { return last_bridge_time.sec_since_epoch(); }
    };
    typedef eosio::multi_index<"addrmappings"_n, address_mapping_row,
        eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<address_mapping_row, checksum256, &address_mapping_row::by_btc_address>>,
        eosio::indexed_by<"byevmaddr"_n, const_mem_fun<address_mapping_row, checksum256, &address_mapping_row::by_evm_address>>,
        eosio::indexed_by<"lastbrdgtime"_n, const_mem_fun<address_mapping_row, uint64_t, &address_mapping_row::by_last_bridge_time>>>
        address_mapping_index;

    struct [[eosio::table]] deposit_row {
        uint64_t id;
        uint64_t permission_id;
        std::vector<name> provider_actors;
        deposit_status status;
        string b_id;
        string wallet_code;
        string btc_address;
        checksum160 evm_address;
        string order_no;
        uint64_t block_height;
        string tx_id;
        uint64_t amount;
        string tx_status;
        string remark_detail;
        uint64_t tx_time_stamp;
        uint64_t create_time_stamp;
        uint64_t primary_key() const { return id; }
        checksum256 by_btc_address() const { return xsat::utils::hash(btc_address); }
        checksum256 by_evm_address() const { return xsat::utils::compute_id(evm_address); }
        checksum256 by_order_no() const { return xsat::utils::hash(order_no); }
        checksum256 by_tx_id() const { return xsat::utils::hash(tx_id); }
    };
    typedef eosio::multi_index<"deposits"_n, deposit_row,
        eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_btc_address>>,
        eosio::indexed_by<"byevmaddr"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_evm_address>>,
        eosio::indexed_by<"byorderno"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_order_no>>,
        eosio::indexed_by<"bytxid"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_tx_id>>>
        deposit_index;

    // table init
    permission_index _permission = permission_index(_self, _self.value);
    address_index _address = address_index(_self, _self.value);
    address_mapping_index _address_mapping = address_mapping_index(_self, _self.value);
    deposit_index _deposit = deposit_index(_self, _self.value);

    void check_permission(const name actor, uint64_t permission_id);
    uint64_t next_address_id();
    uint64_t next_address_mapping_id();
    uint64_t next_deposit_id();
    bool verifyProviders(std::vector<name> requested_actors, std::vector<name> provider_actors);
    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};