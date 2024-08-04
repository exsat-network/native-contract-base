# custody.xsat

## Actions

- addcustody
- updatecusty
- delcustody
- syncstake


## Quickstart

```bash
# addcustody @custody.xsat
$ cleos push action custody.xsat addcustody '["1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3", "0000000000000000000000000000000000000001", "chen.sat", "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN", null ]' -p custody.xsat

# updatecusty @custody.xsat
$ cleos push action custody.xsat updatecusty '["1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3", "chen2.sat"]' -p custody.xsat

# delcustody @custody.xsat
$ cleos push action custody.xsat delcustody '["1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3"]' -p custody.xsat

# syncstake @custody.xsat
$ cleos push action custody.xsat syncstake '[100]' -p custody.xsat

## Table Information

```bash
$ cleos get table custody.xsat custody.xsat globalid
$ cleos get table custody.xsat custody.xsat custodies
```

## Table of Content

- [TABLE `globalid`](#table-globalid)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [TABLE `custodies`](#table-custodies)
  - [scope `get_self()`](#scope-get_self-1)
  - [params](#params-1)
  - [example](#example-1)

## TABLE `globalid`

### scope `get_self()`
### params

- `{uint64_t} custody_id` - The custody id is automatically incremented
- `{uint64_t} last_custody_id` - last processed custody id
- `{uint64_t} last_height` - last processed height

### example

```json
{
  "custody_id": 4,
  "last_custody_id": 3,
  "last_height": 0
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

### example

```json
{
  "id": 1,
  "staker": "1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3",
  "proxy": "0000000000000000000000000000000000000001",
  "validator": "chen.sat",
  "btc_address": "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN",
  "scriptpubkey": "a914cac3a79a829c31b07e6a8450c4e05c4289ab95b887"
}
```
