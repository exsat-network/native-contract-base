# `utxomng.xsat`

## Actions

- Initialize configuration
- Add UTXO
- Delete UTXO
- Add block header
- Delete block header
- Parse UTXO

## Quickstart 

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
$ cleos push action utxo.xsat processblock '{"synchronizer": "alice", "height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", "process_rows":1024}' -p utxomng.xsat
```

## Table Information

```bash
$ cleos get table utxomng.xsat utxomng.xsat chainstate
$ cleos get table utxomng.xsat utxomng.xsat config
$ cleos get table utxomng.xsat utxomng.xsat utxos
$ cleos get table utxomng.xsat utxomng.xsat blocks
$ cleos get table utxomng.xsat utxomng.xsat consensusblk
```

## Table of Content

- [ENUM `parsing_status`](#enum-parsing_status)
- [TABLE `chainstate`](#table-chainstate)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [TABLE `config`](#table-config)
  - [scope `get_self()`](#scope-get_self-1)
  - [params](#params-1)
  - [example](#example-1)
- [TABLE `utxos`](#table-utxos)
  - [scope `get_self()`](#scope-get_self-2)
  - [params](#params-2)
  - [example](#example-2)
- [TABLE `blocks`](#table-blocks)
  - [scope `get_self()`](#scope-get_self-3)
  - [params](#params-3)
  - [example](#example-3)
- [TABLE `block.extra`](#table-blockextra)
  - [scope `height`](#scope-height)
  - [params](#params-4)
  - [example](#example-4)
- [TABLE `consensusblk`](#table-consensusblk)
  - [scope `get_self()`](#scope-get_self-4)
  - [params](#params-5)
  - [example](#example-5)
- [STRUCT `process_block_result`](#struct-process_block_result)
  - [params](#params-6)
  - [example](#example-6)
- [ACTION `init`](#action-init)
  - [params](#params-7)
  - [example](#example-7)
- [ACTION `config`](#action-config)
  - [params](#params-8)
  - [example](#example-8)
- [ACTION `addutxo`](#action-addutxo)
  - [params](#params-9)
  - [example](#example-9)
- [ACTION `delutxo`](#action-delutxo)
  - [params](#params-10)
  - [example](#example-10)
- [ACTION `addblock`](#action-addblock)
  - [params](#params-11)
  - [example](#example-11)
- [ACTION `delblock`](#action-delblock)
  - [params](#params-12)
  - [example](#example-12)
- [ACTION `processblock`](#action-processblock)
  - [params](#params-13)
  - [example](#example-13)
- [ACTION `consensus`](#action-consensus)
  - [params](#params-14)
  - [example](#example-14)

## ENUM `parsing_status`
```
typedef uint8_t parsing_status;
static const parsing_status waiting = 1;
static const parsing_status parsing = 2;
static const parsing_status deleting_data = 3;
static const parsing_status distributing_rewards = 4;
static const parsing_status migrating = 5;
```

## STRUCT `parsing_progress_row`

### params

- `{uint64_t} bucket_id` - the bucket_id currently being parsed
- `{uint64_t} num_transactions` - the number of transactions currently parsing the block
- `{uint64_t} parsed_transactions` - number of transactions currently resolved
- `{uint64_t} parsed_position` - the position of the currently parsed block
- `{uint64_t} parsed_vin` - the current transaction has been resolved to the vin index
- `{uint64_t} parsed_vout` - the current transaction has been resolved to the vout index
- `{name} parser` - the account number of the parsing block
- `{time_point_sec} parsed_expiration_time` - timeout for parsing chunks

### example

```json
{
  "bucket_id": 3,
  "num_transactions": 0,
  "parsed_transactions": 0,
  "parsed_position": 0,
  "parsed_vin": 0,
  "parsed_vout": 0,
  "parser": "alice",
  "parsed_expiration_time": "2024-07-13T14:39:32"
}
```

## TABLE `chainstate`

### scope `get_self()`
### params

- `{uint64_t} num_utxos` - total number of UTXOs
- `{uint64_t} head_height` - header block height for consensus success
- `{uint64_t} irreversible_height` - irreversible block height
- `{uint64_t} irreversible_hash` - irreversible block hash
- `{uint64_t} migrating_height` - block height in migration
- `{uint64_t} migrating_hash` - block hash in migration
- `{uint64_t} migrating_num_utxos` - the total number of UTXOs in the migration
- `{uint64_t} migrated_num_utxos` - number of UTXOs that have been migrated
- `{uint32_t} num_provider_validators` - the number of validators that have endorsed the current parsed block
- `{uint32_t} num_validators_assigned` - the number of validators that have been allocated rewards
- `{name} miner` - block miner account
- `{name} synchronizer` - block synchronizer account
- `{name} parser` - the account number of the parsing block
- `{uint64_t} parsed_height` - parsed block height
- `{uint64_t} parsing_height` - the current height being parsed
- `{map<checksum256, parsing_progress_row>} parsing_progress_of` - parsing progress @see `parsing_progress_row`
- `{uint8_t} status` - parsing status @see `parsing_status`

### example

```json
{
  "num_utxos": 26240,
  "head_height": 840031,
  "irreversible_height": 840002,
  "irreversible_hash": "00000000000000000002c0cc73626b56fb3ee1ce605b0ce125cc4fb58775a0a9",
  "migrating_height": 840003,
  "migrating_hash": "00000000000000000001cfe8671cb9269dfeded2c4e900e365fffae09b34b119",
  "migrating_num_utxos": 16278,
  "migrated_num_utxos": 10000,
  "num_provider_validators": 2,
  "num_validators_assigned": 0,
  "miner": "",
  "synchronizer": "alice",
  "parser": "alice",
  "parsed_height": 840008,
  "parsing_height": 840009,
  "parsing_progress_of": [{
      "first": "00000000000000000000c6075e66b667adcdb8935e6d9a877f5cf140c806ae87",
      "second": {
          "bucket_id": 11,
          "num_utxos": 0,
          "num_transactions": 0,
          "parsed_transactions": 0,
          "parsed_position": 0,
          "parsed_vin": 0,
          "parsed_vout": 0,
          "parser": "alice",
          "parse_expiration_time": "2024-08-08T02:44:43"
      }
      }
  ],
  "status": 5
}
```

## TABLE `config`

### scope `get_self()`
### params

- `{uint16_t} parse_timeout_seconds` - parsing timeout duration
- `{uint16_t} num_validators_per_distribution` - number of endorsing users each time rewards are distributed
- `{uint16_t} num_retain_data_blocks` - number of blocks to retain data
- `{uint16_t} num_txs_per_verification` - the number of tx for each verification (2^n)
- `{uint8_t} num_merkle_layer` - verify the number of merkle levels (log(num_txs_per_verification))
- `{uint16_t} num_miner_priority_blocks` - miners who produce blocks give priority to verifying the number of
blocks

### example

```json
{
  "parse_timeout_seconds": 600,
  "num_validators_per_distribution": 100,
  "num_retain_data_blocks": 100,
  "num_txs_per_verification": 1024,
  "num_merkle_layer": 10,
  "num_miner_priority_blocks": 10
 }
```

## TABLE `utxos`

### scope `get_self()`
### params

- `{uint64_t} id` - primary key
- `{checksum256} txid` - transaction id
- `{uint32_t} index` - vout index
- `{std::vector<uint8_t>} scriptpubkey` - vout's script public key
- `{uint32_t} value` - utxo quantity

### example

```json
{
  "id": 2,
  "txid": "2bb85f4b004be6da54f766c17c1e855187327112c231ef2ff35ebad0ea67c69e",
  "index": 0,
  "scriptpubkey": "51203b8b3ab1453eb47e2d4903b963776680e30863df3625d3e74292338ae7928da1",
  "value": 1797928002
}
```

## TABLE `pendingutxos`

### scope `get_self()`
### params

- `{uint64_t} id` - primary key
- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash
- `{checksum256} txid` - transaction id
- `{uint32_t} index` - vout index
- `{std::vector<uint8_t>} scriptpubkey` - script public key
- `{uint32_t} value` - utxo quantity
- `{name} type` - utxo type (`vin` or `vout`)

### example

```json
{
  "id": 2,
  "height": 840000,
  "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5",
  "txid": "2bb85f4b004be6da54f766c17c1e855187327112c231ef2ff35ebad0ea67c69e",
  "index": 0,
  "scriptpubkey": "51203b8b3ab1453eb47e2d4903b963776680e30863df3625d3e74292338ae7928da1",
  "value": 1797928002,
  "type": "vout"
}
```

## TABLE `blocks`

### scope `get_self()`
### params

- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash
- `{checksum256} cumulative_work` - the cumulative workload of the block
- `{uint32_t} version` - block version
- `{checksum256} previous_block_hash` - hash in internal byte order of the previous block’s header
- `{checksum256} merkle` - the merkle root is derived from the hashes of all transactions included in this block
- `{uint32_t} timestamp` - the block time is a Unix epoch time
- `{uint32_t} bits` - an encoded version of the target threshold this block’s header hash must be less than or
equal to
- `{uint32_t} nonce` - an arbitrary number miners change to modify the header hash in order to produce a hash
less than or

### example

```json
{
  "height": 840011,
  "hash": "00000000000000000002d12efb02bcf70580b2eebf4b775578844640512e30f3",
  "cumulative_work": "0000000000000000000000000000000000000000753f3af9322a2a893cb6ece4",
  "version": 747323392,
  "previous_block_hash": "00000000000000000000da20f7d8e9e6412d4f1d8b62d88264cddbdd48256ba0",
  "merkle": "476eac37dd22a59952c5812f715a3e39b68663e40d80d756ba44d7d59e7d6a0a",
  "timestamp": 1713576761,
  "bits": 386089497,
  "nonce": 1492849681,
 }
```

## TABLE `block.extra`

### scope `height`
### params

- `{uint64_t} bucket_id` - the associated bucket number is used to obtain block data

### example

```json
{
  "bucket_id": 1,
}
```

## TABLE `consensusblk`

### scope `get_self()`
### params

- `{uint64_t} bucket_id` - the associated bucket number is used to obtain block data
- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash
- `{checksum256} cumulative_work` - the cumulative workload of the block
- `{uint32_t} version` - block version
- `{checksum256} previous_block_hash` - hash in internal byte order of the previous block’s header
- `{checksum256} merkle` - the merkle root is derived from the hashes of all transactions included in this block
- `{uint32_t} timestamp` - the block time is a Unix epoch time
- `{uint32_t} bits` - an encoded version of the target threshold this block’s header hash must be less than or
equal to
- `{uint32_t} nonce` - an arbitrary number miners change to modify the header hash in order to produce a hash
less than or
- `{name} miner` - block miner account
- `{name} synchronizer` - block synchronizer account
- `{name} parser` - the last parser of the parsing block
- `{uint64_t} num_utxos` - the total number of vin and vout of the block
- `{bool} irreversible` - is it an irreversible block
- `{time_point_sec} created_at` - created at time

### example

```json
{
  "bucket_id": 5,
  "height": 840003,
  "hash": "00000000000000000001cfe8671cb9269dfeded2c4e900e365fffae09b34b119",
  "cumulative_work": "0000000000000000000000000000000000000000753cc66782a80f6f099fe68c",
  "version": 704643072,
  "previous_block_hash": "00000000000000000002c0cc73626b56fb3ee1ce605b0ce125cc4fb58775a0a9",
  "merkle": "2daee999cac85a7663bbc3a0e24bd7c86e009c005e7d801ef104d134b420179b",
  "timestamp": 1713572633,
  "bits": 386089497,
  "nonce": 213198539,
  "miner": "",
  "synchronizer": "alice",
  "parser": "alice",
  "num_utxos": 16278,
  "irreversible": 1,
  "created_at": "2024-08-13T00:00:00"
}
```

## STRUCT `process_block_result`

### params

- `{uint64_t} height` - block height
- `{checksum256} block_hash` - block hash
- `{string} status` - current parsing status

### example

```json
{
  "height": 840000,
  "block_hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5",
  "status": "parsing",
}
```

## ACTION `init`

- **authority**: `get_self()`

> Initialize block information to start parsing.

### params

- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash
- `{checksum256} cumulative_work` - the cumulative workload of the block

### example

```bash
$ cleos push action utxomng.xsat init '[839999,
"0000000000000000000172014ba58d66455762add0512355ad651207918494ab",
"0000000000000000000000000000000000000000753b8c1eaae701e1f0146360"]' -p utxomng.xsat
```

## ACTION `config`

- **authority**: `get_self()`

> Setting parameters.

### params

- `{uint16_t} parse_timeout_seconds` - parsing timeout duration
- `{uint16_t} num_validators_per_distribution` - number of endorsing users each time rewards are distributed
- `{uint16_t} num_retain_data_blocks` - number of blocks to retain data
- `{uint16_t} num_txs_per_verification` - the number of tx for each verification (2^n)
- `{uint8_t} num_merkle_layer` - verify the number of merkle levels (log(num_txs_per_verification))
- `{uint16_t} num_miner_priority_blocks` - miners who produce blocks give priority to verifying the number of
blocks

### example

```bash
$ cleos push action utxomng.xsat config '[600, 100, 100, 11, 10]' -p utxomng.xsat
```

## ACTION `addutxo`

- **authority**: `get_self()`

> Add utxo data.

### params

- `{uint64_t} id` - primary id
- `{checksum256} txid` - transaction id
- `{uint32_t} index` - vout index
- `{vector<uint8_t>} scriptpubkey` - script public key
- `{uint64_t} value` - utxo quantity

### example

```bash
$ cleos push action utxomng.xsat addutxo '[1, "c323eae524ce3b49f0868396eb9a61bea0e5fb3dc2e52cb46e04c2dba28a3c0d",
1, "001435f6de260c9f3bdee47524c473a6016c0c055cb9", 636813647]' -p utxomng.xsat
```

## ACTION `delutxo`

- **authority**: `get_self()`

> Delete utxo data.

### params

- `{uint64_t} id` - utxo id

### example

```bash
$ cleos push action utxomng.xsat delutxo '[1]' -p utxomng.xsat
```

## ACTION `addblock`

- **authority**: `get_self()`

> Add history block header.

### params

- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash
- `{checksum256} cumulative_work` - the cumulative workload of the block
- `{uint32_t} version` - block version
- `{checksum256} previous_block_hash` - hash in internal byte order of the previous block’s header
- `{checksum256} merkle` - the merkle root is derived from the hashes of all transactions included in this block
- `{uint32_t} timestamp` - the block time is a Unix epoch time
- `{uint32_t} bits` - an encoded version of the target threshold this block’s header hash must be less than or
equal to
- `{uint32_t} nonce` - an arbitrary number miners change to modify the header hash in order to produce a hash
less than or

### example

```bash
$ cleos push action utxomng.xsat addblock '[840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5",
"0000000000000000000172014ba58d66455762add0512355ad651207918494ab", 710926336,
"0000000000000000000172014ba58d66455762add0512355ad651207918494ab",
"031b417c3a1828ddf3d6527fc210daafcc9218e81f98257f88d4d43bd7a5894f", 1713571767, 3932395645, 386089497
]' -p utxomng.xsat
```

## ACTION `delblock`

- **authority**: `get_self()`

> Delete block header.

### params

- `{uint64_t} height` - block height

### example

```bash
$ cleos push action utxomng.xsat delblock '[840000]' -p utxomng.xsat
```

## ACTION `processblock`

- **authority**: `synchronizer`

> Parse utxo

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} process_rows` - number of vins and vouts to be parsed

### example

```bash
$ cleos push action utxomng.xsat processblock '["alice", 1000]' -p alice
```

## ACTION `consensus`

- **authority**: `blksync.xsat` or `blkendt.xsat`

> Reach consensus logic

### params

- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash

### example

```bash
$ cleos push action utxomng.xsat consensus '[840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"]' -p blksync.xsat
```
