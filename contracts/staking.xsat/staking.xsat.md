- [`staking.xsat`](#stakingxsat)
  - [Actions](#actions)
  - [Table Information](#table-information)
  - [TABLE `globalid`](#table-globalid)
    - [scope `get_self()`](#scope-get_self)
    - [params](#params)
    - [example](#example)
  - [TABLE `tokens`](#table-tokens)
    - [scope `get_self()`](#scope-get_self-1)
    - [params](#params-1)
    - [example](#example-1)
  - [TABLE `staking`](#table-staking)
    - [scope `staker`](#scope-staker)
    - [params](#params-2)
    - [example](#example-2)
  - [TABLE `releases`](#table-releases)
    - [scope `staker`](#scope-staker-1)
    - [params](#params-3)
    - [example](#example-3)
  - [ACTION `addtoken`](#action-addtoken)
    - [params](#params-4)
    - [example](#example-4)
  - [ACTION `deltoken`](#action-deltoken)
    - [params](#params-5)
    - [example](#example-5)
  - [ACTION `setstatus`](#action-setstatus)
    - [params](#params-6)
    - [example](#example-6)
  - [ACTION `release`](#action-release)
    - [params](#params-7)
    - [example](#example-7)
  - [ACTION `withdraw`](#action-withdraw)
    - [params](#params-8)
    - [example](#example-8)

# `staking.xsat`

## Actions

```bash
# addtoken @staking.xsat
$ cleos push action staking.xsat addtoken '[{ "sym": "8,BTC", "contract": "btc.xsat" }]' -p staking.xsat

# deltoken @staking.xsat
$ cleos push action staking.xsat deltoken '[1]' -p staking.xsat

# setstatus @staking.xsat
$ cleos push action staking.xsat setstatus '{"id": 1, "disabled_staking": true}' -p staking.xsat

# release @staker
$ cleos push action staking.xsat release '{"staking_id": 1, "staker": "alice", "validator": "alice", "quantity": "1.00000000 BTC"}' -p alice

# withdraw @staker
$ cleos push action staking.xsat withdraw '{"staker": "alice"}' -p alice
```

## Table Information

```bash
$ cleos get table rescmng.xsat staking.xsat globalid
$ cleos get table rescmng.xsat staking.xsat tokens
$ cleos get table rescmng.xsat <staker> staking
$ cleos get table rescmng.xsat <staker> releases
```

## TABLE `globalid`

### scope `get_self()`
### params

- `{uint64_t} staking_id` - the latest staking id
- `{uint64_t} release_id` - the latest release id

### example

```json
{
  "staking_id": 1,
  "release_id": 1
}
```

## TABLE `tokens`

### scope `get_self()`
### params

- `{uint64_t} id` - token id
- `{uint64_t} token` - whitelist token
- `{bool} disabled_staking` - whether to disable staking

### example

```json
{
  "id": 1,
  "token": { "sym": "8,BTC", "contract": "btc.xsat" },
  "disabled_staking": false
}
```

## TABLE `staking`

### scope `staker`
### params

- `{uint64_t} id` - staking id
- `{extended_asset} quantity` - total number of staking

### example

```json
{
  "id": 1,
  "quantity": {"quantity":"1.00000000 BTC", "contract":"btc.xsat"}
}
```

## TABLE `releases`

### scope `staker`
### params

- `{uint64_t} id` - release id
- `{extended_asset} quantity` - unpledged quantity
- `{time_point_sec} expiration_time` - cancel pledge expiration time

### example

```json
{
  "id": 1,
  "quantity": {
      "quantity": "1.00000000 BTC",
      "contract": "btc.xsat"
  },
  "expiration_time": "2024-08-12T08:09:57"
}
```

## ACTION `addtoken`

- **authority**: `get_self()`

> Add whitelist token

### params

- `{extended_symbol} token` - token to add

### example

```bash
$ cleos push action staking.xsat addtoken '[{ "sym": "8,BTC", "contract": "btc.xsat" }]' -p staking.xsat
```

## ACTION `deltoken`

- **authority**: `get_self()`

> Delete whitelist token

### params

- `{uint64_t} id` - token id to be deleted

### example

```bash
$ cleos push action staking.xsat deltoken '[1]' -p staking.xsat
```

## ACTION `setstatus`

- **authority**: `get_self()`

> Set the token’s disabled staking status.

### params

- `{uint64_t} id` - token id
- `{bool} disabled_staking` - whether to disable staking

### example

```bash
$ cleos push action staking.xsat setstatus '[1, true]' -p staking.xsat
```

## ACTION `release`

- **authority**: `staker`

> Cancel the pledge and enter the unlocking period.

### params

- `{uint64_t} staking_id` - staking id
- `{name} staker` - staker account
- `{name} validator` - the validator account to be pledged to
- `{extended_asset} quantity` - unpledged quantity

### example

```bash
$ cleos push action staking.xsat release '[1, "alice", "alice", "1.00000000 BTC"]' -p alice
```

## ACTION `withdraw`

- **authority**: `staker`

> Withdraw expired staking tokens.

### params

- `{name} staker` - staker account

### example

```bash
$ cleos push action staking.xsat withdraw '["alice"]' -p alice
```
