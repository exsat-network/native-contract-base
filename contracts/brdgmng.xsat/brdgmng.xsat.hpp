#pragma once

#include <algorithm>
#include <sstream>
#include <vector>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/singleton.hpp>
#include <btc.xsat/btc.xsat.hpp>
#include <utxomng.xsat/utxomng.xsat.hpp>
#include <bitcoin/utility/address_converter.hpp>

#include "../internal/defines.hpp"
#include "../internal/safemath.hpp"
#include "../internal/utils.hpp"

using namespace eosio;
using namespace std;
using std::vector;

class [[eosio::contract("brdgmng.xsat")]] brdgmng : public contract {
   public:
    using contract::contract;

    typedef uint8_t global_status;
    static const global_status global_status_initiated = 0;
    static const global_status global_status_succeed = 1;
    static const global_status global_status_failed = 2;

    static constexpr name pending_scope = "pending"_n;
    static constexpr name confirmed_scope = "confirmed"_n;

    typedef uint8_t address_status;
    static const address_status address_status_initiated = 0;
    static const address_status address_status_confirmed = 1;
    static const address_status address_status_failed = 2;

    typedef uint8_t withdraw_status;
    static const withdraw_status withdraw_status_initiated = 0;
    static const withdraw_status withdraw_status_withdrawing = 1;
    static const withdraw_status withdraw_status_submitted = 2;
    static const withdraw_status withdraw_status_confirmed = 3;
    static const withdraw_status withdraw_status_failed = 4;
    static const withdraw_status withdraw_status_refund = 5;

    typedef uint8_t tx_status;
    static const tx_status tx_status_unconfirmed = 0;
    static const tx_status tx_status_confirmed = 1;
    static const tx_status tx_status_failed = 2;
    static const tx_status tx_status_frozen = 3;
    static const tx_status tx_status_rollbacked = 4;
    static const tx_status tx_status_unconfirmed_alarm = 5;
    static const tx_status tx_status_error = 6;

    typedef uint8_t order_status;
    static const order_status order_status_processing = 0;
    static const order_status order_status_partially_failed = 1;
    static const order_status order_status_manual_approval = 2;
    static const order_status order_status_failed = 3;
    static const order_status order_status_order_send_out = 4;
    static const order_status order_status_finished = 5;
    static const order_status order_status_canceled = 6;


    static const uint64_t BTC_BASE = 100000000;

    [[eosio::action]]
    void initialize();

    [[eosio::action]]
    void updateconfig(bool deposit_enable, bool withdraw_enable, bool check_uxto_enable, uint64_t limit_amount, uint64_t deposit_fee,
                      uint64_t withdraw_fast_fee, uint64_t withdraw_slow_fee, uint16_t withdraw_merge_count, uint16_t withdraw_timeout_minutes,
                      uint16_t btc_address_inactive_clear_days);

    [[eosio::action]]
    void addperm(const uint64_t id, const vector<name>& actors);

    [[eosio::action]]
    void delperm(const uint64_t id);

    [[eosio::action]]
    void addaddresses(const name& actor, const uint64_t permission_id, string b_id, string wallet_code, const vector<string>& btc_addresses);

    [[eosio::action]]
    void valaddress(const name& actor, const uint64_t permission_id, const uint64_t address_id, const address_status status);

    [[eosio::action]]
    void mappingaddr(const name& actor, const uint64_t permission_id, const checksum160 evm_address);

    [[eosio::action]]
    void deposit(const name& actor, const uint64_t permission_id, const string& b_id, const string& wallet_code, const string& btc_address,
                 const string& order_id, const uint64_t block_height, const string& tx_id, const uint64_t amount, const tx_status tx_status,
                 const optional<string>& remark_detail, const uint64_t tx_time_stamp, const uint64_t create_time_stamp);

    [[eosio::action]]
    void valdeposit(const name& actor, const uint64_t permission_id, const uint64_t deposit_id, const tx_status tx_status, const optional<string>& remark_detail);

    [[eosio::on_notify("*::transfer")]]
    void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    [[eosio::action]]
    void genorderno();

    [[eosio::action]]
    void withdrawinfo(const name& actor, const uint64_t permission_id, const uint64_t withdraw_id, const string& b_id, const string& wallet_code,
                      const string& order_id, const order_status order_status, const uint64_t block_height, const string& tx_id, const optional<string>& remark_detail,
                      const uint64_t tx_time_stamp, const uint64_t create_time_stamp);

    [[eosio::action]]
    void valwithdraw(const name& actor, const uint64_t permission_id, const uint64_t withdraw_id, const withdraw_status withdraw_status,
                     const order_status order_status, const optional<string>& remark_detail);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
#endif

    [[eosio::action]]
    void depositlog() {
        require_auth(get_self());  // todo
    }

    [[eosio::action]]
    void withdrawlog() {
        require_auth(get_self());  // todo
    }

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
        uint64_t withdraw_id;
    };
    typedef singleton<"globalid"_n, global_id_row> global_id_table;
    global_id_table _global_id = global_id_table(_self, _self.value);

    struct [[eosio::table]] config_row {
        bool deposit_enable;
        bool withdraw_enable;
        bool check_uxto_enable;
        uint64_t limit_amount;
        uint64_t deposit_fee;
        uint64_t withdraw_fast_fee;
        uint64_t withdraw_slow_fee;
        uint16_t withdraw_merge_count;
        uint16_t withdraw_timeout_minutes;
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
        uint64_t id;  // different permission group ids are different sources
        std::vector<name> actors;
        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index<"permissions"_n, permission_row> permission_index;

    struct [[eosio::table]] address_row {
        uint64_t id;
        uint64_t permission_id;
        std::vector<name> provider_actors;
        std::vector<uint8_t> statuses;
        uint8_t confirmed_count;
        global_status global_status;
        string b_id;  // business id
        string wallet_code;
        string btc_address;
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
        string b_id;  // business id
        string wallet_code;
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

    struct provider_actor_info {
        name actor;
        tx_status tx_status;

        bool operator==(const provider_actor_info& other) const {
            return actor == other.actor;
        }
    };

    struct [[eosio::table]] deposit_row {
        uint64_t id;
        uint64_t permission_id;
        std::vector<name> provider_actors;
        uint8_t confirmed_count;
        tx_status tx_status;
        global_status global_status;
        string b_id;  // business id
        string wallet_code;
        string btc_address;
        checksum160 evm_address;
        string order_id;
        uint64_t block_height;
        string tx_id;
        uint64_t amount;
        uint64_t fee;
        string remark_detail;
        uint64_t tx_time_stamp;
        uint64_t create_time_stamp;
        uint64_t primary_key() const { return id; }
        checksum256 by_btc_address() const { return xsat::utils::hash(btc_address); }
        checksum256 by_evm_address() const { return xsat::utils::compute_id(evm_address); }
        checksum256 by_order_id() const { return xsat::utils::hash(order_id); }
        checksum256 by_tx_id() const { return xsat::utils::hash(tx_id); }
        uint64_t by_tx_time_stamp() const { return tx_time_stamp; }
    };
    typedef eosio::multi_index<"deposits"_n, deposit_row,
        eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_btc_address>>,
        eosio::indexed_by<"byevmaddr"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_evm_address>>,
        eosio::indexed_by<"byorderid"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_order_id>>,
        eosio::indexed_by<"bytxid"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_tx_id>>,
        eosio::indexed_by<"bytxtime"_n, const_mem_fun<deposit_row, uint64_t, &deposit_row::by_tx_time_stamp>>>
        deposit_index;

    struct [[eosio::table]] withdraw_row {
        uint64_t id;
        uint64_t permission_id;
        std::vector<name> provider_actors;
        uint8_t confirmed_count;
        order_status order_status;
        withdraw_status withdraw_status;
        global_status global_status;
        string b_id;
        string wallet_code;
        string btc_address;
        checksum160 evm_address;
        string gas_level;
        string order_id;
        string order_no;
        uint64_t block_height;
        string tx_id;
        uint64_t amount;
        uint64_t fee;
        string remark_detail;
        uint64_t tx_time_stamp;
        uint64_t create_time_stamp;
        uint64_t primary_key() const { return id; }
        checksum256 by_btc_address() const { return xsat::utils::hash(btc_address); }
        checksum256 by_evm_address() const { return xsat::utils::compute_id(evm_address); }
        checksum256 by_order_id() const { return xsat::utils::hash(order_id); }
        checksum256 by_order_no() const { return xsat::utils::hash(order_no); }
        checksum256 by_tx_id() const { return xsat::utils::hash(tx_id); }
        uint64_t by_tx_time_stamp() const { return tx_time_stamp; }
    };
    typedef eosio::multi_index<"withdraws"_n, withdraw_row,
        eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_btc_address>>,
        eosio::indexed_by<"byevmaddr"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_evm_address>>,
        eosio::indexed_by<"byorderid"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_order_id>>,
        eosio::indexed_by<"byorderno"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_order_no>>,
        eosio::indexed_by<"bytxid"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_tx_id>>,
        eosio::indexed_by<"bytxtime"_n, const_mem_fun<withdraw_row, uint64_t, &withdraw_row::by_tx_time_stamp>>>
        withdraw_index;

    // table init
    permission_index _permission = permission_index(_self, _self.value);
    address_index _address = address_index(_self, _self.value);
    address_mapping_index _address_mapping = address_mapping_index(_self, _self.value);
    deposit_index _deposit = deposit_index(_self, _self.value);
    withdraw_index _withdraw = withdraw_index(_self, _self.value);

    void check_permission(const name& actor, const uint64_t permission_id);
    uint64_t next_address_id();
    uint64_t next_address_mapping_id();
    uint64_t next_deposit_id();
    uint64_t next_withdraw_id();
    bool is_final_tx_status(const tx_status tx_status);
    bool is_final_order_status(const order_status order_status);
    bool verify_providers(const std::vector<name>& requested_actors, const std::vector<name>& provider_actors);
    // bool address_verify_providers(const std::vector<name>& requested_actors, const std::vector<name>& provider_actors);
    // bool verify_providers(const std::vector<name>& requested_actors, const std::vector<provider_actor_info>& provider_actors);
    void do_return(const name& from, const name& contract, const asset& quantity, const string& memo);
    void do_withdraw(const name& from, const name& contract, const asset& quantity, const string& memo);
    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);
    void handle_btc_deposit(const uint64_t amount, const checksum160& evm_address);
    void handle_btc_withdraw(const uint64_t amount);
    string generate_order_no(const std::vector<uint64_t>& ids);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};
