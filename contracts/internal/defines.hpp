#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>

using namespace eosio;

// CONTRACTS
static constexpr name BTC_CONTRACT = "btc.xsat"_n;
static constexpr name EXSAT_CONTRACT = "exsat.xsat"_n;
static constexpr name ERC20_CONTRACT = "erc2o.xsat"_n;
static constexpr name POOL_REGISTER_CONTRACT = "poolreg.xsat"_n;
static constexpr name EVM_UTIL_CONTRACT = "evmutil.xsat"_n;
static constexpr name STAKING_CONTRACT = "staking.xsat"_n;
static constexpr name BLOCK_SYNC_CONTRACT = "blksync.xsat"_n;
static constexpr name BLOCK_ENDORSE_CONTRACT = "blkendt.xsat"_n;
static constexpr name ENDORSER_MANAGE_CONTRACT = "endrmng.xsat"_n;
static constexpr name UTXO_MANAGE_CONTRACT = "utxomng.xsat"_n;
static constexpr name RESOURCE_MANAGE_CONTRACT = "rescmng.xsat"_n;
static constexpr name REWARD_DISTRIBUTION_CONTRACT = "rwddist.xsat"_n;

// SYMBOLS
static constexpr symbol XSAT_SYMBOL = {"XSAT", 8};
static constexpr symbol BTC_SYMBOL = {"BTC", 8};

// CONSTANTS
static constexpr uint32_t DECIMAL = 100000000LL;
static constexpr uint64_t MIN_STAKE_FOR_ENDORSEMENT = 100LL * DECIMAL;
static constexpr uint64_t BTC_SUPPLY = 21000000LL * DECIMAL;
static constexpr uint64_t XSAT_REWARD_PER_BLOCK = 50LL * DECIMAL;

static constexpr uint64_t START_HEIGHT = 839999;
static constexpr uint64_t IRREVERSIBLE_BLOCKS = 6;
static constexpr uint64_t BLOCK_HEADER_SIZE = 80;
static constexpr uint64_t SUBSIDY_HALVING_INTERVAL = 210000;

static constexpr uint64_t DEFAULT_PRODUCTED_BLOCK_LIMIT = 432;
static constexpr uint64_t DEFAULT_NUM_SLOTS = 2;

static constexpr uint8_t STAKE_RELEASE_CYCLE = 28;  // days

static constexpr uint16_t COMMISSION_RATE_BASE = 10000;

static constexpr uint64_t RATE_BASE = 100000000LL;
static constexpr uint64_t SYNCHRONIZER_REWARD_RATE = RATE_BASE / 10;
static constexpr uint64_t MINER_REWARD_RATE = RATE_BASE / 2;
static constexpr uint64_t CONSENSUS_REWARD_RATE = RATE_BASE / 10;

// ENUMS
typedef uint8_t fee_type;
static constexpr fee_type BUY_SLOT = 1;
static constexpr fee_type PUSH_CHUNK = 2;
static constexpr fee_type VERIFY = 3;
static constexpr fee_type ENDORSE = 4;
static constexpr fee_type PARSE = 5;
