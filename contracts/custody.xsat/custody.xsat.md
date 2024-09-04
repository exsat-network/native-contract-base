# custody.xsat

## Actions

- addcustody
- updcustody
- delcustody
- updblkstatus
- onchainsync
- offchainsync


## Quickstart

```bash
# addcustody @custody.xsat
$ cleos push action custody.xsat addcustody '["1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3", "0000000000000000000000000000000000000001", "chen.sat", true, "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN", null ]' -p custody.xsat

# updcustody @custody.xsat
$ cleos push action custody.xsat updcustody '["1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3", "chen2.sat"]' -p custody.xsat

# delcustody @custody.xsat
$ cleos push action custody.xsat delcustody '["1995587ef4e2dd5e6c61a8909110b0ca9a56b1b3"]' -p custody.xsat

# delcustody @custody.xsat
$ cleos push action custody.xsat updblkstatus '[true]' -p custody.xsat

# onchainsync @custody.xsat
$ cleos push action custody.xsat onchainsync '[100]' -p custody.xsat

# offchainsync @custody.xsat
$ cleos push action custody.xsat offchainsync '["1231deb6f5749ef6ce6943a275a1d3e7486f4eae", "1.00000000 BTC"]' -p custody.xsat

## Table Information

```bash
$ cleos get table custody.xsat custody.xsat globals
$ cleos get table custody.xsat custody.xsat custodies
$ cleos get table custody.xsat custody.xsat vaults
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

- `{bool} is_synchronized` - the block sync status
- `{uint64_t} custody_id` - the custody id is automatically incremented
- `{uint64_t} last_custody_id` - last processed custody id
- `{uint64_t} last_height` - last processed height
- `{uint64_t} vault_id` - the vault id is automatically incremented

### example

```json
{
  "is_synchronized": true,
  "custody_id": 4,
  "last_custody_id": 3,
  "last_height": 0,
  "vault_id": 1
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

## TABLE `vaults`

### scope `get_self()`
### params

- `{uint64_t} id` - the vault id
- `{checksum160} staker` - the staker evm address
- `{name} validator` - the validator account
- `{string} btc_address` - the bitcoin address
- `{asset} quantity` - the staking amount{
  "id": 1,
  "staker": "ee37064f01ec9314278f4984ff4b9b695eb91912",
  "validator": "vali.xsat",
  "btc_address": "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN",
  "quantity": "1.00000000 BTC"
}

### example

```json
{
  "id": 1,
  "staker": "ee37064f01ec9314278f4984ff4b9b695eb91912",
  "validator": "vali.xsat",
  "btc_address": "3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN",
  "quantity": "1.00000000 BTC"
}
```
