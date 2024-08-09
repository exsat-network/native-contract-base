# rescmng.xsat

## Actions

- Initialize resource cost configuration
- Deposit fee 
- Withdrawal fee
- Payment for initializing blocks, uploading block data, deleting block data, and validation fees
- Set disable withdrawal status

## Quickstart 

```bash
# init @rescmng.xsat
$ cleos push action rescmng.xsat init '{"fee_account": "fees.xsat", "cost_per_slot": "0.00000020 BTC", "cost_per_upload": "0.00000020 BTC", "cost_per_verification": "0.00000020 BTC", "cost_per_endorsement": "0.00000020 BTC", "cost_per_parse": "0.00000020 BTC"}' -p rescmng.xsat

# setstatus @rescmng.xsat
$ cleos push action rescmng.xsat setstatus '{"disabled_withdraw": true}' -p rescmng.xsat

# pay @blksync.xsat or @blkendt.xsat or @utxomng.xsat or @poolreg.xsat or @blkendt.xsat
$ cleos push action rescmng.xsat pay '{"height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", "type": 1, "quantity": 1}' -p blksync.xsat

# deposit fee @user
$ cleos push action btc.xsat transfer '["alice","rescmng.xsat","1.00000000 BTC", "<receiver>"]' -p alice

# withdraw fee @user
$ cleos push action rescmng.xsat withdraw '{"owner","alice", "quantity": "1.00000000 BTC"}' -p alice
```

## Table Information

```bash
$ cleos get table rescmng.xsat rescmng.xsat config
$ cleos get table rescmng.xsat rescmng.xsat accounts
```

## Table of Content

- [ENUM `fee_type`](#enum-fee_type)
- [TABLE `config`](#table-config)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [TABLE `accounts`](#table-accounts)
  - [scope `get_self()`](#scope-get_self-1)
  - [params](#params-1)
  - [example](#example-1)
- [ACTION `init`](#action-init)
  - [params](#params-2)
  - [example](#example-2)
- [ACTION `setstatus`](#action-setstatus)
  - [params](#params-3)
  - [example](#example-3)
- [ACTION `pay`](#action-pay)
  - [params](#params-4)
  - [example](#example-4)
- [ACTION `withdraw`](#action-withdraw)
  - [params](#params-5)
  - [example](#example-5)

## ENUM `fee_type`
```
typedef uint8_t fee_type;
static constexpr fee_type BUY_SLOT = 1;
static constexpr fee_type PUSH_CHUNK = 2;
static constexpr fee_type VERIFY = 3;
static constexpr fee_type ENDORSE = 4;
static constexpr fee_type PARSE = 5;
```

## STRUCT `CheckResult`

### params

- `{bool} has_auth` - the client's permission correct?
- `{bool} is_exists` - does the client account exist?
- `{asset} balance` - client balance

### example

```json
{
  "has_auth": true,
  "is_exists": true,
  "balance": "0.99999219 BTC"
}
```

## TABLE `config`

### scope `get_self()`
### params

- `{name} fee_account` - account number for receiving handling fees
- `{bool} disabled_withdraw` - whether withdrawal of balance is allowed
- `{asset} cost_per_slot` - cost per slot
- `{asset} cost_per_upload` - cost per upload chunk
- `{asset} cost_per_verification` - the cost of each verification performed
- `{asset} cost_per_endorsement` - the cost of each execution of an endorsement
- `{asset} cost_per_parse` - cost per execution of parsing

### example

```json
{
  "fee_account": "fees.xsat",
  "disabled_withdraw": 0,
  "cost_per_slot": "0.00000001 BTC",
  "cost_per_upload": "0.00000001 BTC",
  "cost_per_verification": "0.00000001 BTC",
  "cost_per_endorsement": "0.00000001 BTC",
  "cost_per_parse": "0.00000001 BTC"
}
```
    
## TABLE `accounts`

### scope `get_self()`
### params

- `{name} owner` - recharge account
- `{asset} balance` - account balance

### example

```json
{
  "owner": "test.xsat",
  "balance": "0.99999765 BTC"
}
```

## ACTION `checkclient`

- **authority**: `anyone`

> Verify that the client is ready.

### params

- `{name} client` - client account
- `{uint8_t} type` - client type 1: synchronizer 2: validator

### example

```bash
$ cleos push action rescmng.xsat checkclient '["alice", 1]' -p alice 
```

## ACTION `init`

- **authority**: `get_self()`

> Modify or update fee configuration.

### params

- `{name} fee_account` - account number for receiving handling fees
- `{asset} cost_per_slot` - cost per slot
- `{asset} cost_per_upload` - cost per upload chunk
- `{asset} cost_per_verification` - the cost of each verification performed
- `{asset} cost_per_endorsement` - the cost of each execution of an endorsement
- `{asset} cost_per_parse` - cost per execution of parsing

### example

```bash
$ cleos push action rescmng.xsat init '["fee.xsat", "0.00000001 BTC",, "0.00000001 BTC", "0.00000001 BTC",
"0.00000001 BTC", "0.00000001 BTC"]' -p rescmng.xsat
```

## ACTION `setstatus`

- **authority**: `get_self()`

> Withdraw balance

### params

- `{bool} disabled_withdraw` - whether to disable withdrawal of balance

### example

```bash
$ cleos push action rescmng.xsat setstatus '[true]' -p rescmng.xsat
```

## ACTION `pay`

- **authority**: `blksync.xsat` or `blkendt.xsat` or `utxomng.xsat` or `poolreg.xsat` or `blkendt.xsat`

> Pay the fee.

### params

- `{uint64_t} height` - block height
- `{hash} hash` - block hash
- `{name} owner` - debited account
- `{fee_type} type` - types of deductions
- `{uint64_t} quantity` - payment quantity

### example

```bash
$ cleos push action rescmng.xsat pay '[840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", "alice", 1, 1]' -p blksync.xsat
```

## ACTION `withdraw`

- **authority**: `owner`

> Withdraw balance

### params

- `{name} owner` - account for withdrawing fees
- `{asset} quantity` - the number of tokens to be withdrawn

### example

```bash
$ cleos push action rescmng.xsat withdraw '["alice", "0.00000001 BTC"]' -p alice
```
