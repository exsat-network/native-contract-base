# brdgmng.xsat
## Actions

- initialize
- updateconfig
- addperm
- delperm
- addaddresses
- valaddress
- mappingaddr
- deposit
- valdeposit
- genorderno
- withdraw
- withdrawinfo
- valwithdraw

## Quickstart

```bash
# initialize @boot.xsat
$ cleos push action brdgmng.xsat initialize '[]' -p boot.xsat

# updateconfig @brdgmng.xsat
$ cleos push action brdgmng.xsat updateconfig '[true, true, false, 1000000, 1000, 2000, 1000, 10, 60, 30]' -p brdgmng.xsat

# addperm @brdgmng.xsat
$ cleos push action brdgmng.xsat addperm '[0, ["actor1.xsat", "actor2.xsat"]]' -p brdgmng.xsat

# delperm @brdgmng.xsat
$ cleos push action brdgmng.xsat delperm '[0]' -p brdgmng.xsat

# addaddresses @actor1.xsat
$ cleos push action brdgmng.xsat addaddresses '[ "actor1.xsat", 0, 1, "walletcode", ["3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN", "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN"] ]' -p actor1.xsat

# valaddress @actor2.xsat
$ cleos push action brdgmng.xsat valaddress '["actor2.xsat", 0, 1, 1]' -p actor2.xsat

# mappingaddr @actor2.xsat
$ cleos push action brdgmng.xsat mappingaddr '["actor1.xsat", 0, "2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6"]' -p actor1.xsat

# deposit @actor1.xsat
$ cleos push action brdgmng.xsat deposit '["actor1.xsat", 0, "b_id", "walletcode", "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN", "order_id", 840000, "tx_id", 1, 1000000, 1, "remark", 1000000, 1000000]' -p actor1.xsat

# genorderno @trigger.xsat
$ cleos push action brdgmng.xsat genorderno '[0]' -p trigger.xsat

# withdraw @user
# memo format: <permission_id>,<evm_address>,<btc_address>,<gas_level>
# gas_level: fast or slow
$ cleos push action btc.xsat transfer '["test.xsat", "brdgmng.xsat", "0.01100000 BTC", "0,0x2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6,18cBEMRxXHqzWWCxZNtU91F5sbUNKhL5PX,fast" ]' -p test.xsat

# withdrawinfo @actor1.xsat
$ cleos push action brdgmng.xsat withdrawinfo '["actor1.xsat", 0, 1, "b_id", "walletcode", "order_id", 1, 840000, "tx_id", null, 1726734182, 1726734182]' -p actor1.xsat

# valwithdraw @actor1.xsat
$ cleos push action brdgmng.xsat valwithdraw '["actor1.xsat", 0, 1, 1, 1, null]' -p actor1.xsat
```

## Table Information

```bash
$ cleos get table brdgmng.xsat brdgmng.xsat boot
$ cleos get table brdgmng.xsat brdgmng.xsat globalid
$ cleos get table brdgmng.xsat brdgmng.xsat configs
$ cleos get table brdgmng.xsat brdgmng.xsat permissions
$ cleos get table brdgmng.xsat <permission_id> statistics
$ cleos get table brdgmng.xsat <permission_id> addresses
$ cleos get table brdgmng.xsat <permission_id> addrmappings
$ cleos get table brdgmng.xsat <permission_id> depositings
$ cleos get table brdgmng.xsat <permission_id> deposits
$ cleos get table brdgmng.xsat <permission_id> withdrawings
$ cleos get table brdgmng.xsat <permission_id> withdraws
```

## Table of Content

- [Actions](#actions)
- [Quickstart](#quickstart)
- [Table Information](#table-information)
- [TABLE `boot`](#table-boot)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [TABLE `globalid`](#table-globalid)
  - [scope `get_self()`](#scope-get_self-1)
  - [params](#params-1)
  - [example](#example-1)
- [TABLE `configs`](#table-configs)
  - [scope `get_self()`](#scope-get_self-2)
  - [params](#params-2)
  - [example](#example-2)
- [TABLE `statistics`](#table-statistics)
  - [scope `get_self()`](#scope-get_self-3)
  - [params](#params-3)
  - [example](#example-3)
- [TABLE `permissions`](#table-permissions)
  - [scope `get_self()`](#scope-get_self-4)
  - [params](#params-4)
  - [example](#example-4)
- [TABLE `addresses`](#table-addresses)
  - [scope `permission_id`](#scope-permission_id)
  - [params](#params-5)
  - [example](#example-5)
- [TABLE `addrmappings`](#table-addrmappings)
  - [scope `permission_id`](#scope-permission_id-1)
  - [params](#params-6)
  - [example](#example-6)
- [TABLE `deposits`](#table-deposits)
  - [scope `permission_id`](#scope-permission_id-2)
  - [params](#params-7)
  - [example](#example-7)
- [TABLE `withdraws`](#table-withdraws)
  - [scope `permission_id`](#scope-permission_id-3)
  - [params](#params-8)
  - [example](#example-8)
- [TABLE `depositings`](#table-depositings)
  - [scope `permission_id`](#scope-permission_id-4)
  - [params](#params-9)
  - [example](#example-9)
- [TABLE `withdrawings`](#table-withdrawings)
  - [scope `permission_id`](#scope-permission_id-5)
  - [params](#params-10)
  - [example](#example-10)

## ENUM `global_status`

```cpp
typedef uint8_t global_status;
static const global_status global_status_initiated = 0;
static const global_status global_status_succeed = 1;
static const global_status global_status_failed = 2;
```

## ENUM `address_status`

```cpp
typedef uint8_t address_status;
static const address_status address_status_initiated = 0;
static const address_status address_status_confirmed = 1;
static const address_status address_status_failed = 2;
```

## ENUM `withdraw_status`

```cpp
typedef uint8_t withdraw_status;
static const withdraw_status withdraw_status_initiated = 0;
static const withdraw_status withdraw_status_withdrawing = 1;
static const withdraw_status withdraw_status_submitted = 2;
static const withdraw_status withdraw_status_confirmed = 3;
static const withdraw_status withdraw_status_failed = 4;
static const withdraw_status withdraw_status_refund = 5;
```

## ENUM `tx_status`

```cpp
typedef uint8_t tx_status;
static const tx_status tx_status_unconfirmed = 0;
static const tx_status tx_status_confirmed = 1;
static const tx_status tx_status_failed = 2;
static const tx_status tx_status_frozen = 3;
static const tx_status tx_status_rollbacked = 4;
static const tx_status tx_status_unconfirmed_alarm = 5;
static const tx_status tx_status_error = 6;
```

## ENUM `order_status`

```cpp
typedef uint8_t order_status;
static const order_status order_status_processing = 0;
static const order_status order_status_partially_failed = 1;
static const order_status order_status_manual_approval = 2;
static const order_status order_status_failed = 3;
static const order_status order_status_order_send_out = 4;
static const order_status order_status_finished = 5;
static const order_status order_status_canceled = 6;
```

## TABLE `boot`

### scope `get_self()`

### params

- `{bool} initialized` - Indicates whether the boot one BTC has been initialized
- `{bool} returned` - Indicates whether the boot one BTC have been returned
- `{asset} quantity` - The amount of asset associated with the boot.xsat

### example

```json
{
  "initialized": true,
  "returned": false,
  "quantity": "1.00000000 BTC"
}
```

## TABLE `globalid`

### scope `get_self()`

### params

- `{uint64_t} permission_id` - The latest permission ID
- `{uint64_t} address_id` - The latest address ID
- `{uint64_t} address_mapping_id` - The latest address mapping ID
- `{uint64_t} deposit_id` - The latest deposit ID
- `{uint64_t} withdraw_id` - The latest withdraw ID

### example

```json
{
  "permission_id": 100,
  "address_id": 200,
  "address_mapping_id": 300,
  "deposit_id": 400,
  "withdraw_id": 500
}
```

## TABLE `configs`

### scope `get_self()`

### params

- `{bool} deposit_enable` - Indicates if deposits are enabled
- `{bool} withdraw_enable` - Indicates if withdrawals are enabled
- `{bool} check_uxto_enable` - Indicates if UTXO checks are enabled
- `{uint64_t} limit_amount` - The limit amount for transactions
- `{uint64_t} deposit_fee` - The fee for deposits
- `{uint64_t} withdraw_fast_fee` - The fee for fast withdrawals
- `{uint64_t} withdraw_slow_fee` - The fee for slow withdrawals
- `{uint16_t} withdraw_merge_count` - The number of withdrawals to merge
- `{uint16_t} withdraw_timeout_minutes` - The timeout in minutes for withdrawals
- `{uint16_t} btc_address_inactive_clear_days` - Days after which inactive BTC addresses are cleared

### example

```json
{
  "deposit_enable": true,
  "withdraw_enable": true,
  "check_uxto_enable": false,
  "limit_amount": 1000000,
  "deposit_fee": 100,
  "withdraw_fast_fee": 200,
  "withdraw_slow_fee": 50,
  "withdraw_merge_count": 5,
  "withdraw_timeout_minutes": 30,
  "btc_address_inactive_clear_days": 90
}
```

## TABLE `statistics`

### scope `get_self()`

### params

- `{uint64_t} total_btc_address_count` - Total number of BTC addresses
- `{uint64_t} mapped_address_count` - Number of mapped addresses
- `{uint64_t} total_deposit_amount` - Total amount deposited
- `{uint64_t} total_withdraw_amount` - Total amount withdrawn

### example

```json
{
  "total_btc_address_count": 1000,
  "mapped_address_count": 750,
  "total_deposit_amount": 5000000,
  "total_withdraw_amount": 4500000
}
```

## TABLE `permissions`

### scope `get_self()`

### params

- `{uint64_t} id` - Unique identifier for the permission group
- `{vector<name>} actors` - List of actors associated with the permission group

### example

```json
{
  "id": 0,
  "actors": ["actor1.xsat", "actor2.xsat"]
}
```

## TABLE `addresses`

### scope `permission_id`

### params

- `{uint64_t} id` - Unique identifier for the address
- `{uint64_t} permission_id` - Associated permission group ID
- `{vector<name>} provider_actors` - List of provider actors
- `{vector<uint8_t>} statuses` - Status codes for the address
- `{uint8_t} confirmed_count` - Number of confirmations
- `{global_status} global_status` - Global status of the address
- `{string} b_id` - Business identifier
- `{string} wallet_code` - Wallet code associated with the address
- `{string} btc_address` - BTC address

### example

```json
{
  "id": 1,
  "permission_id": 0,
  "provider_actors": ["actor1.xsat", "actor2.xsat"],
  "statuses": [0, 1],
  "confirmed_count": 2,
  "global_status": 1,
  "b_id": "business123",
  "wallet_code": "WALLET456",
  "btc_address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"
}
```

## TABLE `addrmappings`

### scope `permission_id`

### params

- `{uint64_t} id` - Unique identifier for the address mapping
- `{string} b_id` - Business identifier
- `{string} wallet_code` - Wallet code associated with the mapping
- `{string} btc_address` - BTC address
- `{checksum160} evm_address` - EVM address associated with the BTC address
- `{time_point_sec} last_bridge_time` - Timestamp of the last bridge operation

### example

```json
{
  "id": 1,
  "b_id": "business123",
  "wallet_code": "WALLET456",
  "btc_address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
  "evm_address": "0xABCDEF1234567890ABCDEF1234567890ABCDEF12",
  "last_bridge_time": "2024-04-27T12:34:56"
}
```

## TABLE `deposits`

### scope `permission_id`

### params

- `{uint64_t} id` - Unique identifier for the deposit
- `{uint64_t} permission_id` - Associated permission group ID
- `{vector<name>} provider_actors` - List of provider actors
- `{uint8_t} confirmed_count` - Number of confirmations
- `{tx_status} tx_status` - Status of the transaction
- `{global_status} global_status` - Global status of the deposit
- `{string} b_id` - Business identifier
- `{string} wallet_code` - Wallet code associated with the deposit
- `{string} btc_address` - BTC address
- `{checksum160} evm_address` - EVM address associated with the BTC address
- `{string} order_id` - Order identifier
- `{uint64_t} block_height` - Block height of the transaction
- `{checksum256} tx_id` - Transaction ID
- `{uint32_t} index` - Transaction index
- `{uint64_t} amount` - Amount deposited
- `{uint64_t} fee` - Fee associated with the deposit
- `{string} remark_detail` - Detailed remarks about the deposit
- `{uint64_t} tx_time_stamp` - Timestamp of the transaction
- `{uint64_t} create_time_stamp` - Timestamp when the deposit was created

### example

```json
{
  "id": 1,
  "permission_id": 100,
  "provider_actors": ["actor1.xsat", "actor2.xsat"],
  "confirmed_count": 2,
  "tx_status": 2,
  "global_status": 1,
  "b_id": "business123",
  "wallet_code": "WALLET456",
  "btc_address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
  "evm_address": "0xABCDEF1234567890ABCDEF1234567890ABCDEF12",
  "order_id": "ORDER789",
  "block_height": 858901,
  "tx_id": "0x54579b87831bf37b9bf36da03f3990029a027f50acde4e86ea9e6d0b9adb377d",
  "tx_index": 1,
  "amount": 1000000,
  "fee": 100,
  "remark_detail": null,
  "tx_time_stamp": 1682501123,
  "create_time_stamp": 1682501120
}
```

## TABLE `withdraws`

### scope `permission_id`

### params

- `{uint64_t} id` - Unique identifier for the withdrawal
- `{uint64_t} permission_id` - Associated permission group ID
- `{vector<name>} provider_actors` - List of provider actors
- `{uint8_t} confirmed_count` - Number of confirmations
- `{order_status} order_status` - Status of the order
- `{withdraw_status} withdraw_status` - Status of the withdrawal
- `{global_status} global_status` - Global status of the withdrawal
- `{string} b_id` - Business identifier
- `{string} wallet_code` - Wallet code associated with the withdrawal
- `{string} btc_address` - BTC address
- `{checksum160} evm_address` - EVM address associated with the BTC address
- `{string} gas_level` - Gas level for the transaction
- `{string} order_id` - Order identifier
- `{string} order_no` - Order number
- `{uint64_t} block_height` - Block height of the transaction
- `{string} tx_id` - Transaction ID
- `{uint64_t} amount` - Amount to withdraw
- `{uint64_t} fee` - Fee associated with the withdrawal
- `{string} remark_detail` - Detailed remarks about the withdrawal
- `{uint64_t} tx_time_stamp` - Timestamp of the transaction
- `{uint64_t} create_time_stamp` - Timestamp when the withdrawal was created
- `{uint64_t} withdraw_time_stamp` - Timestamp when the withdrawal was processed

### example

```json
{
  "id": 1,
  "permission_id": 100,
  "provider_actors": ["actor1.xsat", "actor2.xsat"],
  "confirmed_count": 2,
  "order_status": 2,
  "withdraw_status": 0,
  "global_status": 1,
  "b_id": "business123",
  "wallet_code": "WALLET456",
  "btc_address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
  "evm_address": "ABCDEF1234567890ABCDEF1234567890ABCDEF12",
  "gas_level": "fast",
  "order_id": "ORDER789",
  "order_no": "ORDNO456",
  "block_height": 868901,
  "tx_id": "0x54579b87831bf37b9bf36da03f3990029a027f50acde4e86ea9e6d0b9adb377d",
  "amount": 1000000,
  "fee": 100,
  "remark_detail": "Withdrawal for order ORDNO456",
  "tx_time_stamp": 1682501123,
  "create_time_stamp": 1682501120,
  "withdraw_time_stamp": 1682501150
}
```