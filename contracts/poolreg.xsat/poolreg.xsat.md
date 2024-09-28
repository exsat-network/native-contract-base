# poolreg.xsat

## Actions

- Initialize the synchronizer
- Delete the synchronizer
- Unbind the miner
- Configure financial account and commission rate for the synchronizer
- Purchase a slot
- Claim rewards for validating blocks
- Update the synchronizer with the latest block height


## Quickstart 

```bash
# updateheight @utxomng.xsat
$ cleos push action poolreg.xsat updateheight '{"validator": "alice", "height": 839999, "miners": ["3PiyiAezRdSUQub3ewUXsgw5M6mv6tskGv", "bc1p8k4v4xuz55dv49svzjg43qjxq2whur7ync9tm0xgl5t4wjl9ca9snxgmlt"]}' -p utxomng.xsat

# initpool @poolreg.xsat
$ cleos push action poolreg.xsat initpool '{"synchronizer": "alice", "latest_produced_block_height": 839999, "financial_account": "alice", "miners": [""]}' -p poolreg.xsat

# delpool @poolreg.xsat
$ cleos push action poolreg.xsat delpool '{"synchronizer": "alice"}' -p poolreg.xsat

# unbundle @poolreg.xsat
$ cleos push action poolreg.xsat unbundle '{"id": 1}' -p poolreg.xsat

# config @poolreg.xsat
$ cleos push action poolreg.xsat config '{"synchronizer": "alice", "produced_block_limit": 432}' -p poolreg.xsat

# setfinacct @synchronizer
$ cleos push action poolreg.xsat setfinacct '{"synchronizer": "alice", "financial_account": "alice"}' -p alice

# buyslot @synchronizer
$ cleos push action poolreg.xsat buyslot '{"synchronizer": "alice", "receiver": "alice", "num_slots": 2}' -p alice

# claim @evmutil.xsat or @financial_account
$ cleos push action poolreg.xsat claim '{"synchronizer": "alice"}' -p alice
```

## Table Information

```bash
$ cleos get table poolreg.xsat poolreg.xsat synchronizer
$ cleos get table poolreg.xsat poolreg.xsat miners
```

## Table of Content

- [TABLE `synchronizer`](#table-synchronizer)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [TABLE `miners`](#table-miners)
  - [scope `get_self()`](#scope-get_self-1)
  - [params](#params-1)
  - [example](#example-1)
- [ACTION `updateheight`](#action-updateheight)
  - [params](#params-2)
  - [example](#example-2)
- [ACTION `initpool`](#action-initpool)
  - [params](#params-3)
  - [example](#example-3)
- [ACTION `delpool`](#action-delpool)
  - [params](#params-4)
  - [example](#example-4)
- [ACTION `unbundle`](#action-unbundle)
  - [params](#params-5)
  - [example](#example-5)
- [ACTION `config`](#action-config)
  - [params](#params-6)
  - [example](#example-6)
- [ACTION `buyslot`](#action-buyslot)
  - [params](#params-7)
  - [example](#example-7)
- [ACTION `setfinacct`](#action-setfinacct)
  - [params](#params-8)
  - [example](#example-8)
- [ACTION `claim`](#action-claim)
  - [params](#params-9)
  - [example](#example-9)

## TABLE `synchronizer`

### scope `get_self()`
### params

- `{name} synchronizer` - synchronizer account
- `{name} reward_recipient` - receiving account for receiving rewards
- `{string} memo` - memo when receiving reward transfer
- `{uint16_t} num_slots` - number of slots owned
- `{uint64_t} latest_produced_block_height` - the latest block number
- `{uint16_t} produced_block_limit` - upload block limit, for example, if 432 is set, the upload height needs to be a synchronizer that has produced blocks in 432 blocks before it can be uploaded.
- `{asset} unclaimed` - unclaimed rewards
- `{asset} claimed` - rewards claimed
- `{uint64_t} latest_reward_block` - the latest block number to receive rewards
- `{time_point_sec} latest_reward_time` - latest reward time

### example

```json
{
   "synchronizer": "test.xsat",
   "reward_recipient": "erc2o.xsat",
   "memo": "0x4838b106fce9647bdf1e7877bf73ce8b0bad5f97",
   "num_slots": 2,
   "latest_produced_block_height": 840000,
   "produced_block_limit": 432,
   "unclaimed": "5.00000000 XSAT",
   "claimed": "0.00000000 XSAT",
   "latest_reward_block": 840001,
   "latest_reward_time": "2024-07-13T14:29:32"
}
```

## TABLE `miners`

### scope `get_self()`
### params

- `{uint64_t} id` - primary key
- `{name} synchronizer` - synchronizer account
- `{string} miner` - associated btc miner account

### example

```json
{
   "id": 1,
   "synchronizer": "alice",
   "miner": "3PiyiAezRdSUQub3ewUXsgw5M6mv6tskGv"
}
```

## ACTION `updateheight`

- **authority**: `utxomng.xsat`

> Update synchronizer’s latest block height and add associated btc miners.

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} latest_produced_block_height` - the height of the latest mined block
- `{std::vector<string>} miners` - list of btc accounts corresponding to synchronizer

### example

```bash
$ cleos push action poolreg.xsat updateheight '["alice", 839999, ["3PiyiAezRdSUQub3ewUXsgw5M6mv6tskGv", "bc1p8k4v4xuz55dv49svzjg43qjxq2whur7ync9tm0xgl5t4wjl9ca9snxgmlt"]]' -p poolreg.xsat
```

## ACTION `initpool`

- **authority**: `get_self()`

> Unbind the association between synchronizer and btc miner.

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} latest_produced_block_height` - the height of the latest mined block
- `{string} financial_account` - financial account to receive rewards
- `{std::vector<string>} miners` - list of btc accounts corresponding to synchronizer

### example

```bash
$ cleos push action poolreg.xsat initpool '["alice", 839997, "alice", ["37jKPSmbEGwgfacCr2nayn1wTaqMAbA94Z", "39C7fxSzEACPjM78Z7xdPxhf7mKxJwvfMJ"]]' -p poolreg.xsat
```

## ACTION `delpool`

- **authority**: `get_self()`

> Erase synchronizer.

### params

- `{name} synchronizer` - synchronizer account

### example

```bash
$ cleos push action poolreg.xsat delpool '["alice"]' -p poolreg.xsat
```

## ACTION `unbundle`

- **authority**: `get_self()`

> Unbind the association between synchronizer and btc miner.

### params

- `{uint64_t} id` - primary key of miners table

### example

```bash
$ cleos push action poolreg.xsat unbundle '[1]' -p poolreg.xsat
```

## ACTION `config`

- **authority**: `get_self()`

> Configure synchronizer block output limit.

### params

- `{name} synchronizer` - synchronizer account
- `{uint16_t} produced_block_limit` - upload block limit, for example, if 432 is set, the upload height needs to be a synchronizer that has produced blocks in 432 blocks before it can be uploaded.

### example

```bash
$ cleos push action poolreg.xsat config '["alice", 432]' -p poolreg.xsat
```

## ACTION `buyslot`

- **authority**: `synchronizer`

> Buy slot.

### params

- `{name} synchronizer` - synchronizer account
- `{name} receiver` - the account of the receiving slot
- `{uint16_t} num_slots` - number of slots

### example

```bash
$ cleos push action poolreg.xsat buyslot '["alice", "alice", 2]' -p alice
```

## ACTION `setfinacct`

- **authority**: `synchronizer`

> Configure financial account.

### params

- `{name} synchronizer` - synchronizer account
- `{string} financial_account` - financial account to receive rewards

### example

```bash
$ cleos push action poolreg.xsat setfinacct '["alice", "alice"]' -p alice
```

## ACTION `claim`

- **authority**: `synchronizer->to` or `evmutil.xsat`

> Receive award.

### params

- `{name} synchronizer` - synchronizer account

### example

```bash
$ cleos push action poolreg.xsat claim '["alice"]' -p alice
```
