# `blksync.xsat`

## Actions

- Initialize block bucket
- Sharding of upload chunks
- Delete block shards
- Verify the validity of the block

## Quickstart 

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

## Table Information

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

## Table of Content

- [ENUM `block_status`](#enum-block_status)
- [TABLE `globalid`](#table-globalid)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [STRUCT `verify_info_data`](#struct-verify_info_data)
  - [params](#params-1)
  - [example](#example-1)
- [TABLE `blockbuckets`](#table-blockbuckets)
  - [scope `validator`](#scope-validator)
  - [params](#params-2)
  - [example](#example-2)
- [TABLE `passedindexs`](#table-passedindexs)
  - [scope `height`](#scope-height)
  - [params](#params-3)
  - [example](#example-3)
- [TABLE `blockminer`](#table-blockminer)
  - [scope `height`](#scope-height-1)
  - [params](#params-4)
  - [example](#example-4)
- [TABLE `block.chunk`](#table-blockchunk)
  - [scope `bucket_id`](#scope-bucket_id)
  - [params](#params-5)
  - [example](#example-5)
- [STRUCT `verify_block_result`](#struct-verify_block_result)
  - [params](#params-6)
  - [example](#example-6)
- [ACTION `consensus`](#action-consensus)
  - [params](#params-7)
  - [example](#example-7)
- [ACTION `delchunks`](#action-delchunks)
  - [params](#params-8)
  - [example](#example-8)
- [ACTION `initbucket`](#action-initbucket)
  - [params](#params-9)
  - [example](#example-9)
- [ACTION `pushchunk`](#action-pushchunk)
  - [params](#params-10)
  - [example](#example-10)
- [ACTION `delchunk`](#action-delchunk)
  - [params](#params-11)
  - [example](#example-11)
- [ACTION `delbucket`](#action-delbucket)
  - [params](#params-12)
  - [example](#example-12)
- [ACTION `verify`](#action-verify)
  - [params](#params-13)
  - [example](#example-13)

## ENUM `block_status`
```
typedef uint8_t block_status;
static const block_status uploading = 1;
static const block_status upload_complete = 2;
static const block_status verify_merkle = 3;
static const block_status verify_parent_hash = 4;
static const block_status waiting_miner_verification = 5;
static const block_status verify_fail = 6;
static const block_status verify_pass = 7;
```

## TABLE `globalid`

### scope `get_self()`
### params

- `{uint64_t} bucket_id` - latest bucket_id

### example

```json
{
  "bucket_id": 1
}
```

## STRUCT `verify_info_data`

### params

- `{name} miner` - block miner account
- `{vector<string>} btc_miners` - btc miner account
- `{checksum256} previous_block_hash` - hash in internal byte order of the previous block’s header
- `{checksum256} work` - block workload
- `{checksum256} witness_reserve_value` - witness reserve value in the block
- `{std::optional<checksum256>} - witness commitment in the block
- `{bool} has_witness` - whether any of the transactions in the block contains witness
- `{checksum256} header_merkle` - the merkle root of the block
- `{std::vector<checksum256>} relay_header_merkle` - check header merkle relay data
- `{std::vector<checksum256>} relay_witness_merkle` - check witness merkle relay data
- `{uint64_t} num_transactions` - the number of transactions in the block
- `{uint64_t} processed_position` - the location of the block that has been resolved
- `{uint64_t} processed_transactions` - the number of processed transactions

### example

```json
{
  "miner": "",
  "btc_miners": [
      "1BM1sAcrfV6d4zPKytzziu4McLQDsFC2Qc"
  ],
  "previous_block_hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e",
  "work": "000000000000000000000000000000000000000000004e9235f043634662e0cb",
  "witness_reserve_value": "0000000000000000000000000000000000000000000000000000000000000000",
  "witness_commitment": "aeaa22969e5aac88afd1ac14b19a3ad3a58f5eb0dd151ddddfc749297ebfb020",
  "has_witness": 1,
  "header_merkle": "f3f07d3e4636fa1ae5300b3bc148c361beafd7b3309d30b7ba136d0e59a9a0e5",
  "relay_header_merkle": [
     "d1c9861b0d129b34bb6b733c624bbe0a9b10ff01c6047dced64586ef584987f4",
     "bd95f641a29379f0b5a26961de4bb36bd9568a67ca0615be3fb0a28152ff1806",
     "667eb5d36c67667ae4f10bd30a62e3797e8700e1fbb5e3f754a7526f2b7db1e2",
     "5193ac78b5ef8f570ed24946fbcb96d71284faa27b86296093a93eb5c1cfac06"
  ],
  "relay_witness_merkle": [
     "8a080509ebf6baca260d466c2669200d9b4de750f6a190382c4e8ab6ab6859db",
     "d65d4261be51ca1193e718a6f0cfe6415b6f122f4c3df87861e7452916b45d78",
     "95aa96164225b76afa32a9b2903241067b0ea71228cc2d51b9321148c4e37dd3",
     "0dfca7530a6e950ecdec67c60e5d9574404cc97b333a4e24e3cf2eadd5eb76bd"
  ],
  "num_transactions": 4899,
  "processed_transactions": 4096,
  "processed_position": 1197889
}
```

## TABLE `blockbuckets`

### scope `validator`
### params

- `{uint64_t} bucket_id` - primary key, bucket_id is the scope associated with block.bucket
- `{uint64_t} height` - block height
- `{uint32_t} size` -block size
- `{uint32_t} uploaded_size` - the latest release id
- `{uint8_t} num_chunks` - number of chunks
- `{uint8_t} uploaded_num_chunks` - number of chunks that have been uploaded
- `{vector<uint8_t>} chunk_ids` - the uploaded chunk_id
- `{string} reason` - reason for verification failure
- `{block_status} status` - current block status
- `{time_point_sec} updated_at` - updated at time
- `{std::optional<verify_info_data>} verify_info` - @see struct `verify_info_data`

### example

```json
{
  "bucket_id": 81,
  "height": 840062,
  "hash": "00000000000000000002fc5099a59501b26c34819ac52cc16141275f158c3c6a",
  "size": 1434031,
  "uploaded_size": 1434031,
  "num_chunks": 11,
  "uploaded_num_chunks": 11,
  "chunk_ids": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
  "reason": "",
  "status": 3,
  "updated_at": "2024-08-19T00:00:00",
  "verify_info": {
      "miner": "",
      "btc_miners": [
          "1BM1sAcrfV6d4zPKytzziu4McLQDsFC2Qc"
      ],
      "previous_block_hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e",
      "work": "000000000000000000000000000000000000000000004e9235f043634662e0cb",
      "witness_reserve_value": "0000000000000000000000000000000000000000000000000000000000000000",
      "witness_commitment": "aeaa22969e5aac88afd1ac14b19a3ad3a58f5eb0dd151ddddfc749297ebfb020",
      "has_witness": 1,
      "header_merkle": "f3f07d3e4636fa1ae5300b3bc148c361beafd7b3309d30b7ba136d0e59a9a0e5",
      "relay_header_merkle": [
         "d1c9861b0d129b34bb6b733c624bbe0a9b10ff01c6047dced64586ef584987f4",
         "bd95f641a29379f0b5a26961de4bb36bd9568a67ca0615be3fb0a28152ff1806",
         "667eb5d36c67667ae4f10bd30a62e3797e8700e1fbb5e3f754a7526f2b7db1e2",
         "5193ac78b5ef8f570ed24946fbcb96d71284faa27b86296093a93eb5c1cfac06"
      ],
      "relay_witness_merkle": [
          "8a080509ebf6baca260d466c2669200d9b4de750f6a190382c4e8ab6ab6859db",
          "d65d4261be51ca1193e718a6f0cfe6415b6f122f4c3df87861e7452916b45d78",
          "95aa96164225b76afa32a9b2903241067b0ea71228cc2d51b9321148c4e37dd3",
          "0dfca7530a6e950ecdec67c60e5d9574404cc97b333a4e24e3cf2eadd5eb76bd"
      ],
      "num_transactions": 4899,
      "processed_transactions": 4096,
      "processed_position": 1197889
  }
}
```

## TABLE `passedindexs`

### scope `height`
### params

- `{uint64_t} id` - primary key
- `{checksum256} hash` - block hash
- `{checksum256} cumulative_work` - the cumulative workload of the block
- `{uint64_t} bucket_id` - bucket_id is used to obtain block data
- `{name} synchronizer` - synchronizer account
- `{name} miner` - miner account
- `{time_point_sec} created_at` - created at time

### example

```json
{
  "id": 0,
  "hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e",
  "cumulative_work": "0000000000000000000000000000000000000000753f3af9322a2a893cb6ece4",
  "bucket_id": 80,
  "synchronizer": "test.xsat",
  "miner": "alice",
  "created_at": "2024-08-13T00:00:00"
}
```

## TABLE `blockminer`

### scope `height`
### params

- `{uint64_t} id` - primary key
- `{checksum256} hash` - block hash
- `{name} miner` - block miner account
- `{uint32_t} block_num` - the block number that passed the first verification

### example

```json
{
  "id": 0,
  "hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e",
  "miner": "alice",
  "expired_block_num": 210000
}
```

## TABLE `block.chunk`

### scope `bucket_id`
### params

- `{std::vector<char>} data` - the block chunk for block

### example

```json
{
  "data": ""
}
```

## STRUCT `verify_block_result`

### params

- `{string} status` - verification status
- `{string} reason` - reason for verification failure
- `{checksum256} block_hash` - block hash

### example

```json
{
  "status": "verify_pass",
  "reason": "",
  "block_hash": "000000000000000000029bfa01a7cee248f85425e0d0b198f4947717d4d3441e"
}
```

## ACTION `consensus`

- **authority**: `utxomng.xsat`

> Consensus completion processing logic

### params

- `{uint64_t} height` - block height
- `{name} synchronizer` - synchronizer account
- `{uint64_t} bucket_id` - bucket id

### example

```bash
$ cleos push action blksync.xsat consensus '[840000, "alice", 1]' -p utxomng.xsat
```

## ACTION `delchunks`

- **authority**: `utxomng.xsat`

> Deletion of historical block data after parsing is completed

### params

- `{uint64_t} bucket_id` - bucket_id of block data to be deleted

### example

```bash
$ cleos push action blksync.xsat delchunks '[1]' -p utxomng.xsat
```

## ACTION `initbucket`

- **authority**: `synchronizer`

> Initialize the block information to be uploaded

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash
- `{uint32_t} size` -block size
- `{uint8_t} num_chunks` - number of chunks

### example

```bash
$ cleos push action blksync.xsat initbucket '["alice", 840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", 2325617, 9]' -p alice
```

## ACTION `pushchunk`

- **authority**: `synchronizer`

> Upload block shard data

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash
- `{uint8_t} chunk_id` - chunk id
- `{std::vector<char>} data` - block data to be uploaded

### example

```bash
$ cleos push action blksync.xsat pushchunk '["alice", 840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", 0, ""]' -p alice
```

## ACTION `delchunk`

- **authority**: `synchronizer`

> Delete block shard data

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash
- `{uint8_t} chunk_id` - chunk id

### example

```bash
$ cleos push action blksync.xsat delchunk '["alice", 840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5", 0]' -p alice
```

## ACTION `delbucket`

- **authority**: `synchronizer`

> Delete the entire block data

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash

### example

```bash
$ cleos push action blksync.xsat delbucket '["alice", 840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"]' -p alice
```

## ACTION `verify`

- **authority**: `synchronizer`

> Verify block data

### params

- `{name} synchronizer` - synchronizer account
- `{uint64_t} height` - block height
- `{checksum256} hash` - block hash

### example

```bash
$ cleos push action blksync.xsat verify '["alice", 840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"]' -p alice
```
