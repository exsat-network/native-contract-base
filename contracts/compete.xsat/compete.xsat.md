# compete.xsat

## Actions

- setmindonate
- addround
- activate

## Quickstart

````bash
# setmindonate @compete.xsat
$ cleos push action setmindonate '[2000]' -p compete.xsat

# addround @compete.xsat
$ cleos push action compete.xsat addround '[2, null]' -p compete.xsat

# activate @valia.sat
$ cleos push action compete.xsat activate '["valia.sat"]' -p valia.sat

## Table Information
$ cleos get table compete.xsat compete.xsat globals
$ cleos get table compete.xsat compete.xsat rounds
$ cleos get table compete.xsat compete.xsat activations
````

## Table of Content

- [TABLE `globals`](#table-globals)
  - [scope `get_self()`](#scope-get_self)
  - [params](#params)
  - [example](#example)
- [TABLE `rounds`](#table-rounds)
  - [scope `get_self()`](#scope-get_self-1)
  - [params](#params-1)
  - [example](#example-1)
- [TABLE `activations`](#table-activations)
  - [scope `get_self()`](#scope-get_self-21)
  - [params](#params-2)
  - [example](#example-2)

## TABLE `globals`

### scope `get_global()`

### params

- `{uint64_t} round_id` - the current round identifier
- `{uint16_t} min_donate_rate` - the minimum donate rate for a validator
- `{uint16_t} total_quotas` - the total number of quotas available for all rounds
- `{uint64_t} total_activations` - the total number of activations that have occurred

### example

```json
{
  "round_id": 1,
  "min_donate_rate": 2000,
  "total_quotas": 7,
  "total_activations": 2
}
```

## TABLE `rounds`

### scope `get_rounds()`

### params

- `{uint64_t} round` - the unique identifier for the round
- `{uint16_t} quota` - the number of available quotas for this round
- `{time_point_sec} start_time` - the start time of the round

### example

```
{
  "round": 1,
  "quota": 2,
  "start_time": "2024-10-21T00:00:00"
}
```

## TABLE `activations`

### scope `get_activations()`

### params

- `{name} validator` - the validator account associated with the activation
- `{uint64_t} round` - the round identifier for this activation
- `{time_point_sec} activation_time` - the time when the validator was activated

### example

```
{
  "validator": "vali1.sat",
  "round": 1,
  "activation_time": "2024-10-21T00:00:00"
}
```
