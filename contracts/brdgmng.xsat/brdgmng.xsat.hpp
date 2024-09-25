#pragma once

#include <algorithm>
#include <bitcoin/utility/address_converter.hpp>
#include <btc.xsat/btc.xsat.hpp>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/singleton.hpp>
#include <sstream>
#include <utxomng.xsat/utxomng.xsat.hpp>
#include <vector>

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

    /**
     * ## ACTION `initialize`
     *
     * - **authority**: `boot.xsat`
     *
     * > Initialize issue one BTC to boot.xsat
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat initialize '[]' -p boot.xsat
     * ```
     */
    [[eosio::action]]
    void initialize();

    /**
     * ## ACTION `updateconfig`
     *
     * - **authority**: `get_self()`
     *
     * > Update contract configuration settings
     *
     * ### params
     *
     * - `{bool} deposit_enable` - Enable or disable deposit
     * - `{bool} withdraw_enable` - Enable or disable withdraw
     * - `{bool} check_uxto_enable` - Enable or disable UTXO check
     * - `{uint64_t} limit_amount` - Limit amount for deposit and withdraw
     * - `{uint64_t} deposit_fee` - Fee for deposit
     * - `{uint64_t} withdraw_fast_fee` - Fast withdrawal fee
     * - `{uint64_t} withdraw_slow_fee` - Slow withdrawal fee
     * - `{uint16_t} withdraw_merge_count` - Number of withdrawals to merge
     * - `{uint16_t} withdraw_timeout_minutes` - Timeout for withdrawals in minutes
     * - `{uint16_t} btc_address_inactive_clear_days` - Days to clear inactive BTC addresses
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat updateconfig '[true, true, false, 1000000, 1000, 2000, 1000, 10, 60, 30]' -p
     * brdgmng.xsat
     * ```
     */
    [[eosio::action]]
    void updateconfig(bool deposit_enable, bool withdraw_enable, bool check_uxto_enable, uint64_t limit_amount, uint64_t deposit_fee,
                      uint64_t withdraw_fast_fee, uint64_t withdraw_slow_fee, uint16_t withdraw_merge_count, uint16_t withdraw_timeout_minutes,
                      uint16_t btc_address_inactive_clear_days);

    /**
     * ## ACTION `addperm`
     *
     * - **authority**: `get_self()`
     *
     * > Add a new permission group
     *
     * ### params
     *
     * - `{uint64_t} id` - Unique identifier for the permission
     * - `{vector<name>} actors` - List of actor names associated with the permission
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat addperm '[0, ["actor1.xsat", "actor2.xsat"]]' -p brdgmng.xsat
     * ```
     */
    [[eosio::action]]
    void addperm(const uint64_t id, const vector<name>& actors);

    /**
     * ## ACTION `delperm`
     *
     * - **authority**: `get_self()`
     *
     * > Delete an existing permission group
     *
     * ### params
     *
     * - `{uint64_t} id` - Unique identifier for the permission to delete
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat delperm '[0]' -p brdgmng.xsat
     * ```
     */
    [[eosio::action]]
    void delperm(const uint64_t id);

    /**
     * ## ACTION `addaddresses`
     *
     * - **authority**: `actor`
     *
     * > Add BTC whitelist addresses
     *
     * ### params
     *
     * - `{name} actor` - Actor's name
     * - `{uint64_t} permission_id` - ID of the corresponding permission
     * - `{string} b_id` - Business ID
     * - `{string} wallet_code` - Wallet code
     * - `{vector<string>} btc_addresses` - List of BTC addresses to add
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat addaddresses '[ "actor1.xsat", 0, 1, "walletcode",
     * ["3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN", "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN"] ]' -p actor1.xsat
     * ```
     */
    [[eosio::action]]
    void addaddresses(const name& actor, const uint64_t permission_id, string b_id, string wallet_code, const vector<string>& btc_addresses);

    /**
     * ## ACTION `valaddress`
     *
     * - **authority**: `actor`
     *
     * > Validate a BTC address by actor
     *
     * ### params
     *
     * - `{name} actor` - Actor's name
     * - `{uint64_t} permission_id` - ID of the corresponding permission
     * - `{uint64_t} address_id` - ID of the address to validate
     * - `{address_status} status` - Status of the address
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat valaddress '["actor2.xsat", 0, 1, 1]' -p actor2.xsat
     * ```
     */
    [[eosio::action]]
    void valaddress(const name& actor, const uint64_t permission_id, const uint64_t address_id, const address_status status);

    /**
     * ## ACTION `mappingaddr`
     *
     * - **authority**: `actor`
     *
     * > Map an EVM address to a bitcoin address
     *
     * ### params
     *
     * - `{name} actor` - Actor's name
     * - `{uint64_t} permission_id` - ID of the corresponding permission
     * - `{checksum160} evm_address` - EVM address to map
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat mappingaddr '["actor1.xsat", 0, "2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6"]' -p
     * actor1.xsat
     * ```
     */
    [[eosio::action]]
    void mappingaddr(const name& actor, const uint64_t permission_id, const checksum160& evm_address);

    /**
     * ## ACTION `deposit`
     *
     * - **authority**: `actor`
     *
     * > Record a deposit transaction
     *
     * ### params
     *
     * - `{name} actor` - Actor's name
     * - `{uint64_t} permission_id` - ID of the corresponding permission
     * - `{string} b_id` - Business ID
     * - `{string} wallet_code` - Wallet code
     * - `{string} btc_address` - BTC address for the deposit
     * - `{string} order_id` - Unique order ID for the transaction
     * - `{uint64_t} block_height` - Block height of the transaction
     * - `{checksum256} tx_id` - Transaction ID
     * - `{uint32_t} index` - Transaction index
     * - `{uint64_t} amount` - Amount deposited
     * - `{tx_status} tx_status` - Status of the transaction
     * - `{optional<string>} remark_detail` - Optional remark details
     * - `{uint64_t} tx_time_stamp` - Timestamp of the transaction
     * - `{uint64_t} create_time_stamp` - Timestamp of record creation
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat deposit '["actor1.xsat", 0, "b_id", "walletcode",
     * "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN", "order_id", 840000, "tx_id", 1000000, 1, "remark", 1000000, 1000000]' -p
     * actor1.xsat
     * ```
     */
    [[eosio::action]]
    void deposit(const name& actor, const uint64_t permission_id, const string& b_id, const string& wallet_code, const string& btc_address,
                 const string& order_id, const uint64_t block_height, const checksum256& tx_id, const uint32_t index, const uint64_t amount,
                 const tx_status tx_status, const optional<string>& remark_detail, const uint64_t tx_time_stamp, const uint64_t create_time_stamp);

    /**
     * ## ACTION `valdeposit`
     *
     * - **authority**: `actor`
     *
     * > Validate a deposit transaction
     *
     * ### params
     *
     * - `{name} actor` - Actor's name
     * - `{uint64_t} permission_id` - ID of the corresponding permission
     * - `{uint64_t} deposit_id` - ID of the deposit to validate
     * - `{tx_status} tx_status` - Status of the transaction
     * - `{optional<string>} remark_detail` - Optional remark details
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat valdeposit '["actor1.xsat", 0, 1, 1, null]' -p actor1.xsat
     * ```
     */
    [[eosio::action]]
    void valdeposit(const name& actor, const uint64_t permission_id, const uint64_t deposit_id, const tx_status tx_status,
                    const optional<string>& remark_detail);

    /**
     * ## ACTION `on_transfer`
     *
     * - **authority**: `*`
     *
     * > Handle incoming transfer actions
     *
     * ### params
     *
     * - `{name} from` - Sender's account name
     * - `{name} to` - Receiver's account name
     * - `{asset} quantity` - Amount transferred
     * - `{string} memo` - Transfer memo
     */
    [[eosio::on_notify("*::transfer")]]
    void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    /**
     * ## ACTION `genorderno`
     *
     * - **authority**: `anyone`
     *
     * > Generate a new order number
     * Determine whether to generate an order number based on the number of orders accumulated or the timeout period
     *
     * ### params
     *
     * - `{uint64_t} permission_id` - ID of the corresponding permission
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat genorderno '[0]' -p trigger.xsat
     * ```
     */
    [[eosio::action]]
    void genorderno(const uint64_t permission_id);

    /**
     * ## ACTION `withdrawinfo`
     *
     * - **authority**: `actor`
     *
     * > Record withdrawal information
     *
     * ### params
     *
     * - `{name} actor` - Actor's name
     * - `{uint64_t} permission_id` - ID of the corresponding permission
     * - `{uint64_t} withdraw_id` - ID of the withdrawal
     * - `{string} b_id` - Business ID
     * - `{string} wallet_code` - Wallet code
     * - `{string} order_id` - Unique order ID for the withdrawal
     * - `{order_status} order_status` - Status of the withdrawal order
     * - `{uint64_t} block_height` - Block height of the transaction
     * - `{string} tx_id` - Transaction ID
     * - `{optional<string>} remark_detail` - Optional remark details
     * - `{uint64_t} tx_time_stamp` - Timestamp of the transaction
     * - `{uint64_t} create_time_stamp` - Timestamp of record creation
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat withdrawinfo '["actor1.xsat", 0, 1, "b_id", "walletcode", "order_id", 1, 840000,
     * "tx_id", null, 1726734182, 1726734182]' -p actor1.xsat
     * ```
     */
    [[eosio::action]]
    void withdrawinfo(const name& actor, const uint64_t permission_id, const uint64_t withdraw_id, const string& b_id, const string& wallet_code,
                      const string& order_id, const order_status order_status, const uint64_t block_height, const checksum256& tx_id,
                      const optional<string>& remark_detail, const uint64_t tx_time_stamp, const uint64_t create_time_stamp);

    /**
     * ## ACTION `valwithdraw`
     *
     * - **authority**: `actor`
     *
     * > Validate a withdrawal transaction
     *
     * ### params
     *
     * - `{name} actor` - Actor's name
     * - `{uint64_t} permission_id` - ID of the corresponding permission
     * - `{uint64_t} withdraw_id` - ID of the withdrawal to validate
     * - `{withdraw_status} withdraw_status` - Status of the withdrawal
     * - `{order_status} order_status` - Status of the withdrawal order
     * - `{optional<string>} remark_detail` - Optional remark details
     *
     * ### example
     * ```bash
     * $ cleos push action brdgmng.xsat valwithdraw '["actor1.xsat", 0, 1, 1, 1, null]' -p actor1.xsat
     * ```
     */
    [[eosio::action]]
    void valwithdraw(const name& actor, const uint64_t permission_id, const uint64_t withdraw_id, const withdraw_status withdraw_status,
                     const order_status order_status, const optional<string>& remark_detail);

#ifdef DEBUG
    [[eosio::action]]
    void cleartable(const name table_name, const optional<name> scope, const optional<uint64_t> max_rows);
#endif

    [[eosio::action]]
    void mapaddrlog(const name& actor, const uint64_t permission_id, const uint64_t address_id, const checksum160& evm_address, const string& btc_address) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void depositlog(const uint64_t permission_id, const uint64_t deposit_id, const string& b_id, const string& wallet_code, const global_status global_status,
                    const string& btc_address, const checksum160& evm_address, const string& order_id, const uint64_t block_height, const checksum256& tx_id,
                    const uint32_t index, const uint64_t amount, const uint64_t fee, const optional<string>& remark_detail, const uint64_t tx_time_stamp,
                    const uint64_t create_time_stamp) {
        require_auth(get_self());
    }

    [[eosio::action]]
    void withdrawlog(const uint64_t permission_id, const uint64_t withdraw_id, const string& b_id, const string& wallet_code, const global_status global_status,
                     const string& btc_address, const checksum160& evm_address, const string& order_id, const string& order_no, const order_status order_status,
                     const uint64_t block_height, const checksum256& tx_id, const uint64_t amount, const uint64_t fee, const optional<string>& remark_detail,
                     const uint64_t tx_time_stamp, const uint64_t create_time_stamp) {
        require_auth(get_self());
    }

    using mapaddrlog_action = eosio::action_wrapper<"mapaddrlog"_n, &brdgmng::mapaddrlog>;
    using depositlog_action = eosio::action_wrapper<"depositlog"_n, &brdgmng::depositlog>;
    using withdrawlog_action = eosio::action_wrapper<"withdrawlog"_n, &brdgmng::withdrawlog>;

   private:
    /**
     * ## TABLE `boot`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{bool} initialized` - Indicates whether the boot one BTC has been initialized
     * - `{bool} returned` - Indicates whether the boot one BTC have been returned
     * - `{asset} quantity` - The amount of asset associated with the boot.xsat
     *
     * ### example
     *
     * ```json
     * {
     *   "initialized": true,
     *   "returned": false,
     *   "quantity": "1.00000000 BTC"
     * }
     * ```
     */
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
     * - `{uint64_t} permission_id` - The latest permission ID
     * - `{uint64_t} address_id` - The latest address ID
     * - `{uint64_t} address_mapping_id` - The latest address mapping ID
     * - `{uint64_t} deposit_id` - The latest deposit ID
     * - `{uint64_t} withdraw_id` - The latest withdraw ID
     *
     * ### example
     *
     * ```json
     * {
     *   "permission_id": 100,
     *   "address_id": 200,
     *   "address_mapping_id": 300,
     *   "deposit_id": 400,
     *   "withdraw_id": 500
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

    /**
     * ## TABLE `configs`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{bool} deposit_enable` - Indicates if deposits are enabled
     * - `{bool} withdraw_enable` - Indicates if withdrawals are enabled
     * - `{bool} check_uxto_enable` - Indicates if UTXO checks are enabled
     * - `{uint64_t} limit_amount` - The limit amount for transactions
     * - `{uint64_t} deposit_fee` - The fee for deposits
     * - `{uint64_t} withdraw_fast_fee` - The fee for fast withdrawals
     * - `{uint64_t} withdraw_slow_fee` - The fee for slow withdrawals
     * - `{uint16_t} withdraw_merge_count` - The number of withdrawals to merge
     * - `{uint16_t} withdraw_timeout_minutes` - The timeout in minutes for withdrawals
     * - `{uint16_t} btc_address_inactive_clear_days` - Days after which inactive BTC addresses are cleared
     *
     * ### example
     *
     * ```json
     * {
     *   "deposit_enable": true,
     *   "withdraw_enable": true,
     *   "check_uxto_enable": false,
     *   "limit_amount": 1000000,
     *   "deposit_fee": 100,
     *   "withdraw_fast_fee": 200,
     *   "withdraw_slow_fee": 50,
     *   "withdraw_merge_count": 5,
     *   "withdraw_timeout_minutes": 30,
     *   "btc_address_inactive_clear_days": 90
     * }
     * ```
     */
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

    /**
     * ## TABLE `statistics`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} total_btc_address_count` - Total number of BTC addresses
     * - `{uint64_t} mapped_address_count` - Number of mapped addresses
     * - `{uint64_t} total_deposit_amount` - Total amount deposited
     * - `{uint64_t} total_withdraw_amount` - Total amount withdrawn
     *
     * ### example
     *
     * ```json
     * {
     *   "total_btc_address_count": 1000,
     *   "mapped_address_count": 750,
     *   "total_deposit_amount": 5000000,
     *   "total_withdraw_amount": 4500000
     * }
     * ```
     */
    struct [[eosio::table]] statistics_row {
        uint64_t total_btc_address_count;
        uint64_t mapped_address_count;
        uint64_t total_deposit_amount;
        uint64_t total_withdraw_amount;
    };
    typedef singleton<"statistics"_n, statistics_row> statistics_table;

    /**
     * ## TABLE `permissions`
     *
     * ### scope `get_self()`
     * ### params
     *
     * - `{uint64_t} id` - Unique identifier for the permission group
     * - `{vector<name>} actors` - List of actors associated with the permission group
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 0,
     *   "actors": ["actor1.xsat", "actor2.xsat"]
     * }
     * ```
     */
    struct [[eosio::table]] permission_row {
        uint64_t id;
        std::vector<name> actors;
        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index<"permissions"_n, permission_row> permission_index;

    /**
     * ## TABLE `addresses`
     *
     * ### scope `permission_id`
     * ### params
     *
     * - `{uint64_t} id` - Unique identifier for the address
     * - `{uint64_t} permission_id` - Associated permission group ID
     * - `{vector<name>} provider_actors` - List of provider actors
     * - `{vector<uint8_t>} statuses` - Status codes for the address
     * - `{uint8_t} confirmed_count` - Number of confirmations
     * - `{global_status} global_status` - Global status of the address
     * - `{string} b_id` - Business identifier
     * - `{string} wallet_code` - Wallet code associated with the address
     * - `{string} btc_address` - BTC address
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 1,
     *   "permission_id": 0,
     *   "provider_actors": ["actor1.xsat", "actor2.xsat"],
     *   "statuses": [0, 1],
     *   "confirmed_count": 2,
     *   "global_status": 1,
     *   "b_id": "business123",
     *   "wallet_code": "WALLET456",
     *   "btc_address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"
     * }
     * ```
     */
    struct [[eosio::table]] address_row {
        uint64_t id;
        uint64_t permission_id;
        std::vector<name> provider_actors;
        std::vector<uint8_t> statuses;
        uint8_t confirmed_count;
        global_status global_status;
        string b_id;
        string wallet_code;
        string btc_address;
        uint64_t primary_key() const { return id; }
        checksum256 by_btc_addrress() const { return xsat::utils::hash(btc_address); }
    };
    typedef eosio::multi_index<"addresses"_n, address_row,
                               eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<address_row, checksum256, &address_row::by_btc_addrress>>>
        address_index;

    /**
     * ## TABLE `addrmappings`
     *
     * ### scope `permission_id`
     * ### params
     *
     * - `{uint64_t} id` - Unique identifier for the address mapping
     * - `{string} b_id` - Business identifier
     * - `{string} wallet_code` - Wallet code associated with the mapping
     * - `{string} btc_address` - BTC address
     * - `{checksum160} evm_address` - EVM address associated with the BTC address
     * - `{time_point_sec} last_bridge_time` - Timestamp of the last bridge operation
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 1,
     *   "b_id": "business123",
     *   "wallet_code": "WALLET456",
     *   "btc_address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
     *   "evm_address": "0xABCDEF1234567890ABCDEF1234567890ABCDEF12",
     *   "last_bridge_time": "2024-04-27T12:34:56"
     * }
     * ```
     */
    struct [[eosio::table]] address_mapping_row {
        uint64_t id;
        string b_id;
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

    /**
     * ## TABLE `deposits`
     *
     * ### scope `permission_id`
     * ### params
     *
     * - `{uint64_t} id` - Unique identifier for the deposit
     * - `{uint64_t} permission_id` - Associated permission group ID
     * - `{vector<name>} provider_actors` - List of provider actors
     * - `{uint8_t} confirmed_count` - Number of confirmations
     * - `{tx_status} tx_status` - Status of the transaction
     * - `{global_status} global_status` - Global status of the deposit
     * - `{string} b_id` - Business identifier
     * - `{string} wallet_code` - Wallet code associated with the deposit
     * - `{string} btc_address` - BTC address
     * - `{checksum160} evm_address` - EVM address associated with the BTC address
     * - `{string} order_id` - Order identifier
     * - `{uint64_t} block_height` - Block height of the transaction
     * - `{string} tx_id` - Transaction ID
     * - `{uint64_t} amount` - Amount deposited
     * - `{uint64_t} fee` - Fee associated with the deposit
     * - `{string} remark_detail` - Detailed remarks about the deposit
     * - `{uint64_t} tx_time_stamp` - Timestamp of the transaction
     * - `{uint64_t} create_time_stamp` - Timestamp when the deposit was created
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 1,
     *   "permission_id": 100,
     *   "provider_actors": ["actor1.xsat", "actor2.xsat"],
     *   "confirmed_count": 2,
     *   "tx_status": 2,
     *   "global_status": 1,
     *   "b_id": "business123",
     *   "wallet_code": "WALLET456",
     *   "btc_address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
     *   "evm_address": "0xABCDEF1234567890ABCDEF1234567890ABCDEF12",
     *   "order_id": "ORDER789",
     *   "block_height": 858901,
     *   "tx_id": "0x54579b87831bf37b9bf36da03f3990029a027f50acde4e86ea9e6d0b9adb377d",
     *   "index": 1,
     *   "amount": 1000000,
     *   "fee": 100,
     *   "remark_detail": null,
     *   "tx_time_stamp": 1682501123,
     *   "create_time_stamp": 1682501120
     * }
     * ```
     */
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
        checksum256 tx_id;
        uint32_t index;
        uint64_t amount;
        uint64_t fee;
        string remark_detail;
        uint64_t tx_time_stamp;
        uint64_t create_time_stamp;
        uint64_t primary_key() const { return id; }
        checksum256 by_btc_address() const { return xsat::utils::hash(btc_address); }
        checksum256 by_evm_address() const { return xsat::utils::compute_id(evm_address); }
        checksum256 by_order_id() const { return xsat::utils::hash(order_id); }
        checksum256 by_tx_id() const { return tx_id; }
        uint64_t by_tx_time_stamp() const { return tx_time_stamp; }
    };
    typedef eosio::multi_index<"depositings"_n, deposit_row,
                               eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_btc_address>>,
                               eosio::indexed_by<"byevmaddr"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_evm_address>>,
                               eosio::indexed_by<"byorderid"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_order_id>>,
                               eosio::indexed_by<"bytxid"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_tx_id>>,
                               eosio::indexed_by<"bytxtime"_n, const_mem_fun<deposit_row, uint64_t, &deposit_row::by_tx_time_stamp>>>
        depositing_index;

    typedef eosio::multi_index<"deposits"_n, deposit_row,
                               eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_btc_address>>,
                               eosio::indexed_by<"byevmaddr"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_evm_address>>,
                               eosio::indexed_by<"byorderid"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_order_id>>,
                               eosio::indexed_by<"bytxid"_n, const_mem_fun<deposit_row, checksum256, &deposit_row::by_tx_id>>,
                               eosio::indexed_by<"bytxtime"_n, const_mem_fun<deposit_row, uint64_t, &deposit_row::by_tx_time_stamp>>>
        deposit_index;

    /**
     * ## TABLE `withdraws`
     *
     * ### scope `permission_id`
     * ### params
     *
     * - `{uint64_t} id` - Unique identifier for the withdrawal
     * - `{uint64_t} permission_id` - Associated permission group ID
     * - `{vector<name>} provider_actors` - List of provider actors
     * - `{uint8_t} confirmed_count` - Number of confirmations
     * - `{order_status} order_status` - Status of the order
     * - `{withdraw_status} withdraw_status` - Status of the withdrawal
     * - `{global_status} global_status` - Global status of the withdrawal
     * - `{string} b_id` - Business identifier
     * - `{string} wallet_code` - Wallet code associated with the withdrawal
     * - `{string} btc_address` - BTC address
     * - `{checksum160} evm_address` - EVM address associated with the BTC address
     * - `{string} gas_level` - Gas level for the transaction
     * - `{string} order_id` - Order identifier
     * - `{string} order_no` - Order number
     * - `{uint64_t} block_height` - Block height of the transaction
     * - `{string} tx_id` - Transaction ID
     * - `{uint64_t} amount` - Amount to withdraw
     * - `{uint64_t} fee` - Fee associated with the withdrawal
     * - `{string} remark_detail` - Detailed remarks about the withdrawal
     * - `{uint64_t} tx_time_stamp` - Timestamp of the transaction
     * - `{uint64_t} create_time_stamp` - Timestamp when the withdrawal was created
     * - `{uint64_t} withdraw_time_stamp` - Timestamp when the withdrawal was processed
     *
     * ### example
     *
     * ```json
     * {
     *   "id": 1,
     *   "permission_id": 100,
     *   "provider_actors": ["actor1.xsat", "actor2.xsat"],
     *   "confirmed_count": 2,
     *   "order_status": 2,
     *   "withdraw_status": 0,
     *   "global_status": 1,
     *   "b_id": "business123",
     *   "wallet_code": "WALLET456",
     *   "btc_address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
     *   "evm_address": "ABCDEF1234567890ABCDEF1234567890ABCDEF12",
     *   "gas_level": "fast",
     *   "order_id": "ORDER789",
     *   "order_no": "ORDNO456",
     *   "block_height": 868901,
     *   "tx_id": "0x54579b87831bf37b9bf36da03f3990029a027f50acde4e86ea9e6d0b9adb377d",
     *   "amount": 1000000,
     *   "fee": 100,
     *   "remark_detail": "Withdrawal for order ORDNO456",
     *   "tx_time_stamp": 1682501123,
     *   "create_time_stamp": 1682501120,
     *   "withdraw_time_stamp": 1682501150
     * }
     * ```
     */
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
        checksum256 tx_id;
        uint64_t amount;
        uint64_t fee;
        string remark_detail;
        uint64_t tx_time_stamp;
        uint64_t create_time_stamp;
        uint64_t withdraw_time_stamp;
        uint64_t primary_key() const { return id; }
        checksum256 by_btc_address() const { return xsat::utils::hash(btc_address); }
        checksum256 by_evm_address() const { return xsat::utils::compute_id(evm_address); }
        checksum256 by_order_id() const { return xsat::utils::hash(order_id); }
        checksum256 by_order_no() const { return xsat::utils::hash(order_no); }
        checksum256 by_tx_id() const { return tx_id; }
        uint64_t by_tx_time_stamp() const { return tx_time_stamp; }
        uint64_t by_withdraw_time_stamp() const { return withdraw_time_stamp; }
    };
    typedef eosio::multi_index<"withdrawings"_n, withdraw_row,
                               eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_btc_address>>,
                               eosio::indexed_by<"byevmaddr"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_evm_address>>,
                               eosio::indexed_by<"byorderid"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_order_id>>,
                               eosio::indexed_by<"byorderno"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_order_no>>,
                               eosio::indexed_by<"bytxid"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_tx_id>>,
                               eosio::indexed_by<"bytxtime"_n, const_mem_fun<withdraw_row, uint64_t, &withdraw_row::by_tx_time_stamp>>,
                               eosio::indexed_by<"withdrawtime"_n, const_mem_fun<withdraw_row, uint64_t, &withdraw_row::by_tx_time_stamp>>>
        withdrawing_index;

    typedef eosio::multi_index<"withdraws"_n, withdraw_row,
                               eosio::indexed_by<"bybtcaddr"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_btc_address>>,
                               eosio::indexed_by<"byevmaddr"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_evm_address>>,
                               eosio::indexed_by<"byorderid"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_order_id>>,
                               eosio::indexed_by<"byorderno"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_order_no>>,
                               eosio::indexed_by<"bytxid"_n, const_mem_fun<withdraw_row, checksum256, &withdraw_row::by_tx_id>>,
                               eosio::indexed_by<"bytxtime"_n, const_mem_fun<withdraw_row, uint64_t, &withdraw_row::by_tx_time_stamp>>,
                               eosio::indexed_by<"withdrawtime"_n, const_mem_fun<withdraw_row, uint64_t, &withdraw_row::by_tx_time_stamp>>>
        withdraw_index;

    // table init
    permission_index _permission = permission_index(_self, _self.value);

    void check_first_actor_permission(const name& actor, const uint64_t permission_id);
    void check_permission(const name& actor, const uint64_t permission_id);
    uint64_t next_address_id();
    uint64_t next_address_mapping_id();
    uint64_t next_deposit_id();
    uint64_t next_withdraw_id();
    bool is_final_address_status(const address_status address_status);
    bool is_final_tx_status(const tx_status tx_status);
    bool is_final_order_status(const order_status order_status);
    bool verify_providers(const std::vector<name>& requested_actors, const std::vector<name>& provider_actors);
    bool check_utxo_exist(const checksum256& tx_id, const uint32_t index);
    void do_return(const name& from, const name& contract, const asset& quantity, const string& memo);
    void do_withdraw(const name& from, const name& contract, const asset& quantity, const string& memo);
    void token_transfer(const name& from, const name& to, const extended_asset& value, const string& memo);
    void handle_btc_deposit(const uint64_t permission_id, const uint64_t amount, const checksum160& evm_address);
    void handle_btc_withdraw(const uint64_t permission_id, const uint64_t amount);
    string generate_order_no(const std::vector<uint64_t>& ids);

#ifdef DEBUG
    template <typename T>
    void clear_table(T& table, uint64_t rows_to_clear);
#endif
};
