# `xsatstk.xsat`

## Actions

- Set the xsat token's staking disable status
- Unstake tokens
- Withdraw tokens that have reached their expiration time

## Quickstart 

```bash
# setstatus @xsatstk.xsat
$ cleos push action xsatstk.xsat setstatus '{"disabled_staking": true}' -p xsatstk.xsat

# deposit @xsatstk.xsat
$ cleos push action exsat.xsat transfer '{"disabled_staking": true}' -p xsatstk.xsat

# staking @staker
$ cleos push action exsat.xsat transfer '{"from":"alice","to":"xsatstk.xsat","quantity":"1.00000000 XSAT", "memo":"alice"}' -p alice

# release @staker
$ cleos push action xsatstk.xsat release '{"staker": "alice", "validator": "alice", "quantity": "1.00000000 XSAT"}' -p alice

# withdraw @staker
$ cleos push action xsatstk.xsat withdraw '{"staker": "alice"}' -p alice
```

## Table Information

```bash
$ cleos get table xsatstk.xsat xsatstk.xsat globalid
$ cleos get table xsatstk.xsat xsatstk.xsat config
$ cleos get table xsatstk.xsat staking staking -L <staker> -U <staker>
$ cleos get table xsatstk.xsat <staker> releases
```

## Table of Content


- [TABLE `globalid`](#table-globalid)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [TABLE `config`](#table-config)
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
- [ACTION `setstatus`](#action-setstatus)
  - [params](#params-4)
  - [example](#example-4)
- [ACTION `release`](#action-release)
  - [params](#params-5)
  - [example](#example-5)
- [ACTION `withdraw`](#action-withdraw)
  - [params](#params-6)
  - [example](#example-6)


## TABLE `globalid`

### scope `get_self()`
### params

- `{uint64_t} release_id` - the latest release id

### example

```json
{
  "release_id": 1
}
```

## TABLE `config`

### scope `get_self()`
### params

- `{bool} disabled_staking` - the staking status of xsat,  `true` to disable pledge

### example

```json
{
  "disabled_staking": true
}
```

## TABLE `staking`

### scope `staker`
### params

- `{name} staker` - the staker account
- `{extended_asset} quantity` - total number of staking

### example

```json
{
  "staker": "alice",
  "quantity":"1.00000000 XSAT"
}
```

## TABLE `releases`

### scope `staker`
### params

- `{uint64_t} id` - release id
- `{asset} quantity` - unpledged quantity
- `{time_point_sec} expiration_time` - cancel pledge expiration time

### example

```json
{
  "id": 1,
  "quantity": "1.00000000 XSAT",
  "expiration_time": "2024-08-12T08:09:57"
}
```

## ACTION `setstatus`

- **authority**: `get_self()`

> Set the staking status of xsat

### params

- `{bool} disabled_staking` - whether to disable staking

### example

```bash
$ cleos push action xsatstk.xsat setstatus '[true]' -p xsatstk.xsat
```

## ACTION `release`

- **authority**: `staker`

> Cancel the pledge and enter the unlocking period.

### params

- `{name} staker` - staker account
- `{name} validator` - the validator account to be pledged to
- `{extended_asset} quantity` - unpledged quantity

### example

```bash
$ cleos push action staking.xsat release '["alice", "alice", "1.00000000 XSAT"]' -p alice
```

## ACTION `withdraw`

- **authority**: `staker`

> Withdraw expired staking tokens.

### params

- `{name} staker` - staker account

### example

```bash
$ cleos push action xsatstk.xsat withdraw '["alice"]' -p alice
```
