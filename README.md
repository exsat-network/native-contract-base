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

#### Actions

```bash
# initpool @poolreg.xsat
$ cleos push action poolreg.xsat initpool '{"synchronizer": "alice", "latest_produced_block_height": 839999, "financial_account": "alice", "miners": [""]}' -p poolreg.xsat

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

#### Table Information

```bash
$ cleos get table poolreg.xsat poolreg.xsat synchronizer
$ cleos get table poolreg.xsat poolreg.xsat miners
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
