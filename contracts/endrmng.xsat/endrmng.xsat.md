- [`endrmng.xsat`](#endrmngxsat)
  - [Actions](#actions)
  - [Table Information](#table-information)
  - [CONSTANT `WHITELIST_TYPES`](#constant-whitelist_types)
  - [TABLE `globalid`](#table-globalid)
    - [scope `get_self()`](#scope-get_self)
    - [params](#params)
    - [example](#example)
  - [TABLE `whitelist`](#table-whitelist)
    - [scope `proxyreg` or `evmcaller`](#scope-proxyreg-or-evmcaller)
    - [params](#params-1)
    - [example](#example-1)
  - [TABLE `evmproxys`](#table-evmproxys)
    - [scope `get_self()`](#scope-get_self-1)
    - [params](#params-2)
    - [example](#example-2)
  - [TABLE `evmstakers`](#table-evmstakers)
    - [scope `get_self()`](#scope-get_self-2)
    - [params](#params-3)
    - [example](#example-3)
  - [TABLE `stakers`](#table-stakers)
    - [scope `get_self()`](#scope-get_self-3)
    - [params](#params-4)
    - [example](#example-4)
  - [TABLE `validators`](#table-validators)
    - [scope `get_self()`](#scope-get_self-4)
    - [params](#params-5)
    - [example](#example-5)
  - [TABLE `stat`](#table-stat)
    - [scope `get_self()`](#scope-get_self-5)
    - [params](#params-6)
    - [example](#example-6)
  - [ACTION `addwhitelist`](#action-addwhitelist)
    - [params](#params-7)
    - [example](#example-7)
  - [ACTION `delwhitelist`](#action-delwhitelist)
    - [params](#params-8)
    - [example](#example-8)
  - [ACTION `addevmproxy`](#action-addevmproxy)
    - [params](#params-9)
    - [example](#example-9)
  - [ACTION `delevmproxy`](#action-delevmproxy)
    - [params](#params-10)
    - [example](#example-10)
  - [ACTION `setstatus`](#action-setstatus)
    - [params](#params-11)
    - [example](#example-11)
  - [ACTION `regvalidator`](#action-regvalidator)
    - [params](#params-12)
    - [example](#example-12)
  - [ACTION `proxyreg`](#action-proxyreg)
    - [params](#params-13)
    - [example](#example-13)
  - [ACTION `config`](#action-config)
    - [params](#params-14)
    - [example](#example-14)
  - [ACTION `stake`](#action-stake)
    - [params](#params-15)
    - [example](#example-15)
  - [ACTION `unstake`](#action-unstake)
    - [params](#params-16)
    - [example](#example-16)
  - [ACTION `newstake`](#action-newstake)
    - [params](#params-17)
    - [example](#example-17)
  - [ACTION `claim`](#action-claim)
    - [params](#params-18)
    - [example](#example-18)
  - [ACTION `evmstake`](#action-evmstake)
    - [params](#params-19)
    - [example](#example-19)
  - [ACTION `evmunstake`](#action-evmunstake)
    - [params](#params-20)
    - [example](#example-20)
  - [ACTION `evmnewstake`](#action-evmnewstake)
    - [params](#params-21)
    - [example](#example-21)
  - [ACTION `evmclaim`](#action-evmclaim)
    - [params](#params-22)
    - [example](#example-22)
  - [ACTION `vdrclaim`](#action-vdrclaim)
    - [params](#params-23)
    - [example](#example-23)
  - [STRUCT `reward_details_row`](#struct-reward_details_row)
    - [params](#params-24)
    - [example](#example-24)
  - [ACTION `distribute`](#action-distribute)
    - [params](#params-25)
    - [example](#example-25)

# `endrmng.xsat`

## Actions

```bash
# addevmproxy @endrmng.xsat
$ cleos push action endrmng.xsat addevmproxy '{"proxy": "e4d68a77714d9d388d8233bee18d578559950cf5"}' -p endrmng.xsat

# delevmproxy @endrmng.xsat
$ cleos push action endrmng.xsat delevmproxy '{"proxy": "e4d68a77714d9d388d8233bee18d578559950cf5"}' -p endrmng.xsat

# addwhitelist @endrmng.xsat type = ["proxyreg", "evmcaller"]
$ cleos push action endrmng.xsat addwhitelist '{"type": "proxyreg", "account": "alice"}' -p endrmng.xsat

# delwhitelist @endrmng.xsat type = ["proxyreg", "evmcaller"]
$ cleos push action endrmng.xsat delwhitelist '{"type": "proxyreg", "account": "alice"}' -p endrmng.xsat

# setstatus @endrmng.xsat
$ cleos push action endrmng.xsat setstatus '{"validator": "alice", "disabled_staking": true}' -p endrmng.xsat

# regvalidator @validator
$ cleos push action endrmng.xsat regvalidator '{"validator": "alice", "financial_account": "alice"}' -p alice

# proxyreg @proxy
$ cleos push action endrmng.xsat proxyreg '{"proxy": "alice", "validator": "alice", "financial_account": "alice"}' -p alice

# config @validator decimal = 10000
$ cleos push action endrmng.xsat config '{"validator": "alice", "commission_rate": 2000, "financial_account": "alice"}' -p alice

# newstake @staker
$ cleos push action endrmng.xsat newstake '{"staker": "alice", "old_validator": "alice", "new_validator": "bob", "quantity": "0.00000020 BTC"}' -p alice

# claim @staker
$ cleos push action endrmng.xsat claim '{"staker": "alice", "validator": "alice"}' -p alice

# evmstake @caller whitelist["evmcaller"]
$ cleos push action endrmng.xsat evmstake '{"caller": "evmutil.xsat", "proxy": "e4d68a77714d9d388d8233bee18d578559950cf5", "staker": "bbbbbbbbbbbbbbbbbbbbbbbb5530ea015b900000",  "validator": "alice", "quantity": "0.00000020 BTC"}' -p alice

# evmunstake @caller whitelist["evmcaller"] 
$ cleos push action endrmng.xsat evmunstake '{"caller": "evmutil.xsat", "proxy": "e4d68a77714d9d388d8233bee18d578559950cf5", "staker": "bbbbbbbbbbbbbbbbbbbbbbbb5530ea015b900000",  "validator": "alice", "quantity": "0.00000020 BTC"}' -p evmutil.xsat 

# evmnewstake @caller whitelist["evmcaller"] 
$ cleos push action endrmng.xsat evmnewstake '{"caller": "evmutil.xsat", "proxy": "e4d68a77714d9d388d8233bee18d578559950cf5", "staker": "bbbbbbbbbbbbbbbbbbbbbbbb5530ea015b900000",  "old_validator": "alice", "new_validator": "bob", "quantity": "0.00000020 BTC"}' -p evmutil.xsat

# evmclaim @caller whitelist["evmcaller"] 
$ cleos push action endrmng.xsat evmclaim '{"caller": "evmutil.xsat", "proxy": "e4d68a77714d9d388d8233bee18d578559950cf5", "staker": "bbbbbbbbbbbbbbbbbbbbbbbb5530ea015b900000",  "validator": "alice"}' -p evmutil.xsat

# vdrclaim @validator
$ cleos push action endrmng.xsat vdrclaim '{"validator": "alice"}' -p alice 

# distribute @rwddist.xsat
$ cleos push action endrmng.xsat distribute '{"height": 840000, [{"validator": "alice", "staking_rewards": "0.00000020
XSAT", "consensus_rewards": "0.00000020 XSAT"}]}' -p rwddist.xsat
```

## Table Information

```bash
$ cleos get table endrmng.xsat evmcaller whitelist 
$ cleos get table endrmng.xsat proxyreg whitelist 
$ cleos get table endrmng.xsat endrmng.xsat evmproxys 
$ cleos get table endrmng.xsat endrmng.xsat evmstakers 
$ cleos get table endrmng.xsat endrmng.xsat stakers 
$ cleos get table endrmng.xsat endrmng.xsat validators 
$ cleos get table endrmng.xsat endrmng.xsat stat
```

## CONSTANT `WHITELIST_TYPES`
```
const std::set<name> WHITELIST_TYPES = {"proxyreg"_n, "evmcaller"_n};
```

## TABLE `globalid`

### scope `get_self()`
### params

- `{uint64_t} staking_id` - the latest staking id

### example

```json
{
  "staking_id": 1
}
```

## TABLE `whitelist`

### scope `proxyreg` or `evmcaller`
### params

- `{name} account` - whitelist account

### example

```json
{
  "account": "alice"
}
```

## TABLE `evmproxys`

### scope `get_self()`
### params

- `{uint64_t} id` - evm proxy id
- `{checksum160} proxy` - evm agent account

### example

```json
{
  "id": 1,
  "proxy": "bb776ae86d5996908af46482f24be8ccde2d4c41"
}
```

## TABLE `evmstakers`

### scope `get_self()`
### params

- `{uint64_t} id` - evm staker id
- `{checksum160} proxy` - proxy account
- `{checksum160} staker` - staker account
- `{name} validator` - validator account
- `{asset} quantity` - total number of staking
- `{uint64_t} stake_debt` - amount of requested stake debt
- `{asset} staking_reward_unclaimed` - amount of stake unclaimed rewards
- `{asset} staking_reward_claimed` - amount of stake claimed rewards
- `{uint64_t} consensus_debt` - amount of requested consensus debt
- `{asset} consensus_reward_unclaimed` - amount of consensus unclaimed rewards
- `{asset} consensus_reward_claimed` - amount of consensus claimed rewards

### example

```json
{
  "id": 4,
  "proxy": "bb776ae86d5996908af46482f24be8ccde2d4c41",
  "staker": "e4d68a77714d9d388d8233bee18d578559950cf5",
  "validator": "alice",
  "quantity": "0.10000000 BTC",
  "stake_debt": 1385452,
  "staking_reward_unclaimed": "0.00000000 XSAT",
  "staking_reward_claimed": "0.00000000 XSAT",
  "consensus_debt": 173181,
  "consensus_reward_unclaimed": "0.00000000 XSAT",
  "consensus_reward_claimed": "0.00000000 XSAT"
}
```

## TABLE `stakers`

### scope `get_self()`
### params

- `{uint64_t} id` - staker id
- `{name} staker` - staker account
- `{name} validator` - validator account
- `{asset} quantity` - total number of staking
- `{uint64_t} stake_debt` - amount of requested stake debt
- `{asset} staking_reward_unclaimed` - amount of stake unclaimed rewards
- `{asset} staking_reward_claimed` - amount of stake claimed rewards
- `{uint64_t} consensus_debt` - amount of requested consensus debt
- `{asset} consensus_reward_unclaimed` - amount of consensus unclaimed rewards
- `{asset} consensus_reward_claimed` - amount of consensus claimed rewards

### example

```json
{
  "id": 2,
  "staker": "alice",
  "validator": "alice",
  "quantity": "0.10000000 BTC",
  "stake_debt": 1385452,
  "staking_reward_unclaimed": "0.00000000 XSAT",
  "staking_reward_claimed": "0.00000000 XSAT",
  "consensus_debt": 173181,
  "consensus_reward_unclaimed": "0.00000000 XSAT",
  "consensus_reward_claimed": "0.00000000 XSAT"
}
```

## TABLE `validators`

### scope `get_self()`
### params

- `{name} owner` - staker id
- `{name} reward_recipient` - receiving account for receiving rewards
- `{string} memo` - memo when receiving reward transfer
- `{uint64_t} commission_rate` - commission ratio, decimal is 10^4
- `{name} staker` - staker account
- `{name} validator` - validator account
- `{uint128_t} stake_acc_per_share` - staking rewards earnings per share
- `{uint128_t} consensus_acc_per_share` - consensus reward earnings per share
- `{asset} staking_reward_unclaimed` - unclaimed staking rewards
- `{asset} staking_reward_claimed` - amount of stake claimed rewards
- `{asset} consensus_reward_unclaimed` - amount of consensus unclaimed rewards
- `{asset} consensus_reward_claimed` - amount of consensus claimed rewards
- `{asset} total_consensus_reward` - total consensus rewards
- `{asset} consensus_reward_balance` - consensus reward balance
- `{asset} total_staking_reward` - total staking rewards
- `{asset} staking_reward_balance` - staking reward balance
- `{time_point_sec} latest_staking_time` - latest staking or unstaking time
- `{uint64_t} latest_reward_block` - latest reward block
- `{time_point_sec} latest_reward_time` - latest reward time
- `{bool} disabled_staking` - whether to disable staking

### example

```json
{
  "owner": "alice",
  "reward_recipient": "erc2o.xsat",
  "memo": "0x5EB954fB68159e0b7950936C6e1947615b75C895",
  "commission_rate": 0,
  "quantity": "102.10000000 BTC",
  "stake_acc_per_share": "39564978",
  "consensus_acc_per_share": "4945621",
  "staking_reward_unclaimed": "0.00000000 XSAT",
  "staking_reward_claimed": "0.00000000 XSAT",
  "consensus_reward_unclaimed": "0.00000000 XSAT",
  "consensus_reward_claimed": "0.00000000 XSAT",
  "total_consensus_reward": "5.04700642 XSAT",
  "consensus_reward_balance": "5.04700642 XSAT",
  "total_staking_reward": "40.37605144 XSAT",
  "staking_reward_balance": "40.37605144 XSAT",
  "latest_staking_time": "2024-07-13T09:16:26",
  "latest_reward_block": 840001,
  "latest_reward_time": "2024-07-13T14:29:32",
  "disabled_staking": 0
 }
```

## TABLE `stat`

### scope `get_self()`
### params

- `{asset} total_staking` - total staking amount

### example

```json
{
  "total_staking": "100.40000000 BTC"
}
```

## ACTION `addwhitelist`

- **authority**: `get_self()`

> Add whitelist account

### params

- `{name} type` - whitelist type @see `WHITELIST_TYPES`
- `{name} account` - whitelist account

### example

```bash
$ cleos push action endrmng.xsat addwhitelist '["proxyreg", "alice"]' -p endrmng.xsat
```

## ACTION `delwhitelist`

- **authority**: `get_self()`

> Delete whitelist account

### params

- `{name} type` - whitelist type @see `WHITELIST_TYPES`
- `{name} account` - whitelist account

### example

```bash
$ cleos push action endrmng.xsat addwhitelist '["proxyreg", "alice"]' -p endrmng.xsat
```

## ACTION `addevmproxy`

- **authority**: `get_self()`

> Add evm proxy account

### params

- `{checksum160} proxy` - proxy account

### example

```bash
$ cleos push action endrmng.xsat addevmproxy '["bb776ae86d5996908af46482f24be8ccde2d4c41"]' -p endrmng.xsat
```

## ACTION `delevmproxy`

- **authority**: `get_self()`

> Delete evm proxy account

### params

- `{checksum160} proxy` - proxy account

### example

```bash
$ cleos push action endrmng.xsat delevmproxy '["bb776ae86d5996908af46482f24be8ccde2d4c41"]' -p endrmng.xsat
```

## ACTION `setstatus`

- **authority**: `get_self()`

> Set validator staking status

### params

- `{name} validator` - validator account
- `{bool} disabled_staking` - whether to disable staking

### example

```bash
$ cleos push action endrmng.xsat setstatus '["alice",  true]' -p alice
```

## ACTION `regvalidator`

- **authority**: `validator`

> Registering a validator

### params

- `{name} validator` - validator account
- `{name} financial_account` - financial accounts
- `{uint64_t} commission_rate` - commission ratio, decimal is 10^4

### example

```bash
$ cleos push action endrmng.xsat regvalidator '["alice", "alice", 1000]' -p alice
```

## ACTION `proxyreg`

- **authority**: `proxy`

> Proxy account registration validator

### params

- `{name} proxy` - proxy account
- `{name} validator` - validator account
- `{string} financial_account` - financial accounts
- `{uint64_t} commission_rate` - commission ratio, decimal is 10^4

### example

```bash
$ cleos push action endrmng.xsat proxyreg '["test.xsat", "alice", "alice",  1000]' -p test.xsat
```

## ACTION `config`

- **authority**: `validator`

> Validator sets commission ratio and financial account

### params

- `{name} validator` - validator account
- `{uint64_t} commission_rate` - commission ratio, decimal is 10^4
- `{string} financial_account` - financial accounts

### example

```bash
$ cleos push action endrmng.xsat config '["alice",  1000, "alice"]' -p alice
```

## ACTION `stake`

- **authority**: `staking.xsat`

> Staking to validator

### params

- `{name} staker` - staker account
- `{name} validator` - validator account
- `{asset} quantity` - staking amount

### example

```bash
$ cleos push action endrmng.xsat stake '["alice",  "alice", "1.00000000 BTC"]' -p staking.xsat
```

## ACTION `unstake`

- **authority**: `staking.xsat`

> Unstaking from a validator

### params

- `{name} staker` - staker account
- `{name} validator` - validator account
- `{asset} quantity` - cancel staking amount

### example

```bash
$ cleos push action endrmng.xsat unstake '["alice",  "alice", "1.00000000 BTC"]' -p staking.xsat
```

## ACTION `newstake`

- **authority**: `staking.xsat`

> Staking to a new validator

### params

- `{name} staker` - staker account
- `{name} old_validator` - old validator account
- `{name} new_validator` - new validator account
- `{asset} quantity` - the amount of stake transferred to the new validator

### example

```bash
$ cleos push action endrmng.xsat newstake '["alice",  "alice", "bob", "1.00000000 BTC"]' -p staking.xsat
```

## ACTION `claim`

- **authority**: `staker`

> Claim staking rewards

### params

- `{name} staker` - staker account
- `{name} validator` - validator account

### example

```bash
$ cleos push action endrmng.xsat claim '["alice",  "bob"]' -p alice
```

## ACTION `evmstake`

- **authority**: `caller`

> Staking to validator via evm

### params

- `{name} caller` - caller account
- `{checksum160} proxy` - evm proxy account
- `{checksum160} staker` - evm staker account
- `{name} validator` - validator account
- `{asset} quantity` - staking amount

### example

```bash
$ cleos push action endrmng.xsat evmstake '["evmutil.xsat", "bb776ae86d5996908af46482f24be8ccde2d4c41",
"e4d68a77714d9d388d8233bee18d578559950cf5",  "alice", "1.00000000 BTC"]' -p evmutil.xsat
```

## ACTION `evmunstake`

- **authority**: `caller`

> Unstake from validator via evm

### params

- `{name} caller` - caller account
- `{checksum160} proxy` - evm proxy account
- `{checksum160} staker` - evm staker account
- `{name} validator` - validator account
- `{asset} quantity` - staking amount

### example

```bash
$ cleos push action endrmng.xsat evmunstake '["evmutil.xsat", "bb776ae86d5996908af46482f24be8ccde2d4c41",
"e4d68a77714d9d388d8233bee18d578559950cf5",  "alice", "1.00000000 BTC"]' -p evmutil.xsat
```

## ACTION `evmnewstake`

- **authority**: `caller`

> Staking to a new validator via evm

### params

- `{name} caller` - caller account
- `{checksum160} proxy` - evm proxy account
- `{checksum160} staker` - evm staker account
- `{name} old_validator` - old validator account
- `{name} new_validator` - new validator account
- `{asset} quantity` - staking amount

### example

```bash
$ cleos push action endrmng.xsat evmnewstake '["evmutil.xsat", "bb776ae86d5996908af46482f24be8ccde2d4c41",
"e4d68a77714d9d388d8233bee18d578559950cf5",  "alice", "bob", "1.00000000 BTC"]' -p evmutil.xsat
```

## ACTION `evmclaim`

- **authority**: `caller`

> Claim staking rewards through evm

### params

- `{name} caller` - caller account
- `{checksum160} proxy` - evm proxy account
- `{checksum160} staker` - evm staker account
- `{name} validator` - validator account

### example

```bash
$ cleos push action endrmng.xsat evmclaim '["evmutil.xsat", "bb776ae86d5996908af46482f24be8ccde2d4c41",
"e4d68a77714d9d388d8233bee18d578559950cf5",  "alice"]' -p evmutil.xsat
```

## ACTION `vdrclaim`

- **authority**: `validator->reward_recipient` or `evmutil.xsat`

> Validator Receive Rewards

### params

- `{name} validator` - validator account

### example

```bash
$ cleos push action endrmng.xsat vdrclaim '["alice"]' -p alice
```

## STRUCT `reward_details_row`

### params

- `{name} validator` - validator account
- `{asset} staking_rewards` - staking rewards
- `{asset} consensus_rewards` - consensus rewards

### example

```json
{
  "validator": "alice",
  "staking_rewards": "0.00000010 XSAT",
  "consensus_rewards": "0.00000020 XSAT"
}
```

## ACTION `distribute`

- **authority**: `rwddist.xsat`

> Distributing validator rewards

### params

- `{uint64_t} height` - validator account
- `{vector<reward_details_row>} rewards` - validator account

### example

```bash
$ cleos push action endrmng.xsat distribute '[840000, [{"validator": "alice", "staking_rewards": "0.00000020
XSAT", "consensus_rewards": "0.00000020 XSAT"}]]' -p rwddist.xsat
```
