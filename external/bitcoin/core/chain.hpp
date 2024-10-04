#pragma once

namespace bitcoin::core {
    enum base58_type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,

        MAX_BASE58_TYPES
    };

    enum buried_deployment : int16_t {
        // buried deployments get negative values to avoid overlap with DeploymentPos
        DEPLOYMENT_HEIGHTINCB = std::numeric_limits<int16_t>::min(),
        DEPLOYMENT_CLTV,
        DEPLOYMENT_DERSIG,
        DEPLOYMENT_CSV,
        DEPLOYMENT_SEGWIT,
    };

    struct block {
        uint64_t height;
        eosio::checksum256 hash;
        eosio::checksum256 previous_block_hash;
        eosio::checksum256 cumulative_work;
        uint32_t timestamp;
        uint32_t bits;
    };

    struct Params {
        int BIP34_height;
        int BIP65_height;
        int BIP66_height;
        int CSV_height;
        int Segwit_height;
        uint32_t pow_limit;
        bool pow_allow_min_difficulty_blocks;
        bool enforce_BIP94;
        bool pow_no_retargeting;
        uint64_t pow_target_spacing;
        uint64_t pow_target_timespan;

        std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
        std::string bech32_hrp;

        uint64_t difficulty_adjustment_interval() const { return pow_target_timespan / pow_target_spacing; }

        int deployment_height(buried_deployment dep) const {
            switch (dep) {
                case DEPLOYMENT_HEIGHTINCB:
                    return BIP34_height;
                case DEPLOYMENT_CLTV:
                    return BIP65_height;
                case DEPLOYMENT_DERSIG:
                    return BIP66_height;
                case DEPLOYMENT_CSV:
                    return CSV_height;
                case DEPLOYMENT_SEGWIT:
                    return Segwit_height;
            }  // no default case, so the compiler can warn about missing cases
            return std::numeric_limits<int>::max();
        }
    };

};  // namespace bitcoin::core