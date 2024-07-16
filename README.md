# contract-of-consensus

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/exsat-network/contract-of-consensus/blob/main/LICENSE)

## Contracts

| name                                                  | description                    |
| ----------------------------------------------------- | ------------------------------ |
| [btc.xsat](https://bloks.io/account/btc.xsat)         | Btc Token Contract             |
| [xsat.xsat](https://bloks.io/account/xsat.xsat)       | Xsat Contract                  |
| [poolreg.xsat](https://bloks.io/account/poolreg.xsat) | Pool Register Contract         |
| [rescmng.xsat](https://bloks.io/account/rescmng.xsat) | Resource Manage Contract       |
| [utxomng.xsat](https://bloks.io/account/utxomng.xsat) | UTXO Manage Contract           |
| [rwddist.xsat](https://bloks.io/account/rwddist.xsat) | Reward Distribution Contract   |
| [blkendt.xsat](https://bloks.io/account/blkendt.xsat) | Block Endorse Contract         |
| [blksync.xsat](https://bloks.io/account/blksync.xsat) | Block Synchronization Contract |
| [endrmng.xsat](https://bloks.io/account/endrmng.xsat) | Endorsement Manage Contract    |
| [staking.xsat](https://bloks.io/account/staking.xsat) | Staking Contract               |

## Quickstart

### `poolreg.xsat`

## TABLE `synchronizer`

### scope `get_self()`
### params

- `{name} synchronizer` - synchronizer account
- `{name} reward_recipient` - receiving account for receiving rewards
- `{string} memo` - memo when receiving reward transfer
- `{uint16_t} num_slots` - number of slots owned
- `{uint64_t} latest_produced_block_height` - the latest block number
- `{uint16_t} produced_block_limit` - upload block limit, for example, if 432 is set, the upload height needs to
be a synchronizer that has produced blocks in 432 blocks before it can be uploaded.
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

- **authority**: `blksync.xsat`

> Update synchronizer’s latest block height and add associated btc miners.

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} latest_produced_block_height` - the height of the latest mined block
- `{std::vector<string>} miners` - list of btc accounts corresponding to synchronizer

### example

```bash
$ cleos push action poolreg.xsat updateheight '["alice", 839999, ["3PiyiAezRdSUQub3ewUXsgw5M6mv6tskGv",
"bc1p8k4v4xuz55dv49svzjg43qjxq2whur7ync9tm0xgl5t4wjl9ca9snxgmlt"]]' -p poolreg.xsat
```


### `rescmng.xsat`

#### Actions

```bash
# config @rescmng.xsat
$ cleos push action rescmng.xsat init '{"fee_account": "fees.xsat", "cost_per_slot": "0.00000020 BTC", "cost_per_upload": "0.00000020 BTC", "cost_per_verification": "0.00000020 BTC", "cost_per_endorsement": "0.00000020 BTC", "cost_per_parse": "0.00000020 BTC"}' -p rescmng.xsat

# deposit fee @user
$ cleos push action btc.xsat transfer '["alice","rescmng.xsat","1.00000000 BTC", "<receiver>"]' -p alice

# withdraw fee @user
$ cleos push action rescmng.xsat withdraw '{"owner","alice", "quantity": "1.00000000 BTC"}' -p alice
```

#### Table Information

```bash
$ cleos get table rescmng.xsat rescmng.xsat config
$ cleos get table rescmng.xsat rescmng.xsat accounts
```

### `blksync.xsat`

#### Actions

```bash
# initbucket @synchronizer
$ cleos push action blksync.xsat initbucket '{"synchronizer": "alice", "height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", "block_size": 2325617, "num_chunks": 11}' -p alice

# pushchunk @synchronizer
$ cleos push action blksync.xsat pushchunk '{"synchronizer": "alice", "height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", "chunk_id": 0, "data": "<data>"}' -p alice

# delchunk @synchronizer
$ cleos push action blksync.xsat delchunk '{"synchronizer": "alice", "height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", "chunk_id": 0}' -p alice

# delbucket @synchronizer
$ cleos push action blksync.xsat delbucket '{"synchronizer": "alice", "height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"}' -p alice

# verify @synchronizer
$ cleos push action blksync.xsat verify '{"synchronizer": "alice", "height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"}' -p alice
```

#### Table Information

```bash
$ cleos get table blksync.xsat <synchronizer> blockbuckets
# by status
$ cleos get table blksync.xsat <synchronizer> blockbuckets --index 2 --key-type uint64_t -U <status> -L <status>
# by blockid
$ cleos get table blksync.xsat <synchronizer> blockbuckets --index 3 --key-type sha256 -U <blockid> -L <blockid>

$ cleos get table blksync.xsat <height> passedindexs
# by hash
$ cleos get table blksync.xsat <height> block.chunk  --index 3 --key-type sha256 -U <hash> -L <hash>

$ cleos get table blksync.xsat <height> blockminer
```

### `blkendt.xsat`

#### Actions

```bash
# endorse @validator
$ cleos push action endtmng.xsat endorse '{"validator": "alice", "height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"}' -p alice
```

#### Table Information

```bash
$ cleos get table endtmng.xsat <height> endorsements
# by hash
$ cleos get table endtmng.xsat <height> endorsements --index 2 --key-type sha256 -L <hash> -U <hash>
```

### `rwddist.xsat`

#### Table Information

```bash
$ cleos get table rwddist.xsat rwddist.xsat rewardlogs
```

### `utxomng.xsat`

#### Actions

```bash

# init @utxomng.xsat
$ cleos push action utxo.xsat init '{"height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", "cumulative_work": "0.00000020 BTC"}' -p utxomng.xsat

# config @utxomng.xsat
$ cleos push action utxo.xsat config '{"parse_timeout_seconds": 600, "num_validators_per_distribution": 100, "num_retain_data_blocks": 100, "num_merkle_layer": 10, "num_miner_priority_blocks": 10}' -p utxomng.xsat

# addutxo @utxomng.xsat
$ cleos push action utxo.xsat addutxo '{"id": 1, "txid": "76a914536ffa992491508dca0354e52f32a3a7a679a53a88ac", "index": 1, "to": "18cBEMRxXHqzWWCxZNtU91F5sbUNKhL5PX", "value": 4075061499}' -p utxomng.xsat

# delutxo @utxomng.xsat
$ cleos push action utxo.xsat delutxo '{"id": 1}' -p utxomng.xsat

# addblock @utxomng.xsat
$ cleos push action utxo.xsat addblock '{"height":839999,"hash":"000000000000000003e251c7387c2cd5aeac480327a234ec11c9b8382455db0d","cumulative_work":"0000000000000000000000000000000000000000002fa415a1793f473a706960","version":536870912,"previous_block_hash":"000000000000000003e6820666f1a47c7771f18b03f9d24c2896a3d7356a5e3c","merkle":"da1dcebe6d631251a31969b7ed6ba55258d113be3e7ef3ef3343d8f7fc9c1702","timestamp":1479777318,"bits":386089497,"nonce":2635095261}' -p utxomng.xsat

# delblock @utxomng.xsat
$ cleos push action utxo.xsat delblock '{"height":839999}' -p utxomng.xsat

# processblock @alice
$ cleos push action utxo.xsat processblock '{"synchronizer": "alice", "process_rows":1024}' -p utxomng.xsat
```

#### Table Information

```bash
$ cleos get table utxomng.xsat utxomng.xsat chainstate
$ cleos get table utxomng.xsat utxomng.xsat config
$ cleos get table utxomng.xsat utxomng.xsat utxos
$ cleos get table utxomng.xsat utxomng.xsat blocks
$ cleos get table utxomng.xsat utxomng.xsat consensusblk
```

### `endrmng.xsat`

#### Actions

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
$ cleos push action endrmng.xsat setstatus '{"validator": "alice", "disabled_staking": "alice"}' -p endrmng.xsat

# regvalidator @validator
$ cleos push action endrmng.xsat regvalidator '{"validator": "alice", "financial_account": "alice"}' -p alice

# proxyreg @proxy
$ cleos push action endrmng.xsat proxyreg '{"proxy": "alice", "validator": "alice", "financial_account": "alice"}' -p alice

# config @validator decimal= 10000
$ cleos push action endrmng.xsat config '{"validator": "alice", "commission_rate": 2000, "financial_account": "alice"}' -p alice

# setstatus @endrmng.xsat
$ cleos push action endrmng.xsat setstatus '{"validator": "alice", "disabled_staking": true}' -p endrmng.xsat

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
```

#### Table Information

```bash
$ cleos get table endrmng.xsat evmcaller whitelist 
$ cleos get table endrmng.xsat proxyreg whitelist 
$ cleos get table endrmng.xsat endrmng.xsat evmproxys 
$ cleos get table endrmng.xsat endrmng.xsat evmstakers 
$ cleos get table endrmng.xsat endrmng.xsat stakers 
$ cleos get table endrmng.xsat endrmng.xsat validators 
$ cleos get table endrmng.xsat endrmng.xsat stat
```
