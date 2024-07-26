# `blkendt.xsat`

## Actions

- Endorse a block

## Quickstart 

```bash
# erase @utxomng.xsat
$ cleos push action endtmng.xsat erase '{"height": 840000}' -p utxomng.xsat
# endorse @validator
$ cleos push action endtmng.xsat endorse '{"validator": "alice", "height": 840000, "hash": "0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"}' -p alice
```

## Table Information

```bash
$ cleos get table endtmng.xsat <height> endorsements
# by hash
$ cleos get table endtmng.xsat <height> endorsements --index 2 --key-type sha256 -L <hash> -U <hash>
```

## Table of Content

- [STRUCT `validator_info`](#struct-validator_info)
  - [example](#example)
- [TABLE `config`](#table-config)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example-1)
- [TABLE `endorsements`](#table-endorsements)
  - [scope `height`](#scope-height)
  - [params](#params-1)
  - [example](#example-2)
- [ACTION `config`](#action-config)
  - [params](#params-2)
  - [example](#example-3)
- [ACTION `endorse`](#action-endorse)
  - [params](#params-3)
  - [example](#example-4)
- [ACTION `erase`](#action-erase)
  - [params](#params-4)
  - [example](#example-5)

## STRUCT `validator_info`

- `{name} account` - validator account
- `{uint64_t} staking` - the validator's staking amount

### example

```json
{
  "account": "test.xsat",
  "staking": "10200000000"
}
```

## TABLE `config`

### scope `get_self()`
### params

- `{bool} disabled_endorse` - whether to disable endorsement

### example

```json
{
  "disabled_endorse": false
}
```

## TABLE `endorsements`

### scope `height`
### params

- `{uint64_t} id` - primary key
- `{checksum256} hash` - endorsement block hash
- `{std::vector<validator_info>} requested_validators` - list of unendorsed validators
- `{std::vector<validator_info>} provider_validators` - list of endorsed validators

### example

```json
{
  "id": 0,
  "hash": "00000000000000000000da20f7d8e9e6412d4f1d8b62d88264cddbdd48256ba0",
  "requested_validators": [],
  "provider_validators": [{
      "account": "test.xsat",
      "staking": "10200000000"
     }
  ]
}
```

## ACTION `config`

- **authority**: `get_self()`

> Configure endorsement status

### params

- `{bool} disabled_endorse` - whether to disable endorsement

### example

```bash
$ cleos push action blkendt.xsat config '[true]' -pblkendt.xsat
```


## ACTION `endorse`

- **authority**: `validator`

> Endorsement block

### params

- `{name} validator` - validator account
- `{uint64_t} height` - to endorse the height of the block
- `{checksum256} hash` - to endorse the hash of the block

### example

```bash
$ cleos push action blkendt.xsat endorse '["alice", 840000,
"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"]' -p alice
```

## ACTION `erase`

- **authority**: `utxomng.xsat`

> To erase high endorsements

### params

- `{uint64_t} height` - to endorse the height of the block

### example

```bash
$ cleos push action blkendt.xsat erase '[840000]' -p utxomng.xsat
```
