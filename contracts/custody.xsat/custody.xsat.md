# custody.xsat

## Actions

- addcustody
- delcustody
- creditstake


## Quickstart

```bash
# addcustody @custody.xsat
$ cleos push action custody.xsat addcustody '["1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3", "0000000000000000000000000000000000000001", "chen.sat", "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN", null ]' -p custody.xsat

# delcustody @custody.xsat
$ cleos push action custody.xsat delcustody '["1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3"]' -p custody.xsat

# offchainsync @custody.xsat
$ cleos push action custody.xsat creditstake '["1231deb6f5749ef6ce6943a275a1d3e7486f4eae", 10000000000]' -p custody.xsat

## Table Information

```bash
$ cleos get table custody.xsat custody.xsat globals
$ cleos get table custody.xsat custody.xsat custodies
```

## Table of Content

- [TABLE `globals`](#table-globals)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [TABLE `custodies`](#table-custodies)
  - [scope `get_self()`](#scope-get_self-1)
  - [params](#params-1)
  - [example](#example-1)

## TABLE `globals`

### scope `get_self()`
### params

- `{uint64_t} custody_id` - the custody id is automatically incremented

### example

```json
{
  "custody_id": 4
}
```

## TABLE `custodies`

### scope `get_self()`
### params

- `{uint64_t} id` - the custody id
- `{checksum160} staker` - the staker evm address
- `{checksum160} proxy` - the proxy evm address
- `{name} validator` - the validator account
- `{string} btc_address` - the bitcoin address
- `{vector<uint8_t>} scriptpubkey` - the scriptpubkey
- `{uint64_t} value` - the total utxo value
- `{time_point_sec} latest_stake_time` - the latest stake time

### example

```json
{
  "id": 1,
  "staker": "1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3",
  "proxy": "0000000000000000000000000000000000000001",
  "validator": "chen.sat",
  "btc_address": "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN",
  "scriptpubkey": "a914cac3a79a829c31b07e6a8450c4e05c4289ab95b887",
  "value": 100000000,
  "latest_stake_time": "2021-09-01T00:00:00"
}
```
