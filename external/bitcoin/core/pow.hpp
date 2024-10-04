#pragma once

#include <bitcoin/core/chain.hpp>
#include <bitcoin/utility/types.hpp>

namespace bitcoin::core {

    typedef std::optional<block>(GetAncestor)(const uint64_t, const optional<checksum256>);

    uint32_t calculate_next_work_required(const block& prev_block, const uint32_t first_block_time,
                                          GetAncestor get_ancestor, const bitcoin::core::Params& params) {
        // Limit adjustment step
        int64_t actual_timespan = prev_block.timestamp - first_block_time;

        if (actual_timespan < params.pow_target_timespan / 4) actual_timespan = params.pow_target_timespan / 4;
        if (actual_timespan > params.pow_target_timespan * 4) actual_timespan = params.pow_target_timespan * 4;

        // Retarget
        bitcoin::uint256_t bn_new;

        // Special difficulty rule for Testnet4
        if (params.enforce_BIP94) {
            // Here we use the first block of the difficulty period. This way
            // the real difficulty is always preserved in the first block as
            // it is not allowed to use the min-difficulty exception.
            int height_first = prev_block.height - (params.difficulty_adjustment_interval() - 1);
            auto pindex_first = get_ancestor(height_first, std::nullopt);
            bn_new = bitcoin::compact::decode(pindex_first->bits);
        } else {
            bn_new = bitcoin::compact::decode(prev_block.bits);
        }

        bn_new *= actual_timespan;
        bn_new /= params.pow_target_timespan;
        auto pow_limit = params.pow_limit;
        if (bn_new > pow_limit) bn_new = pow_limit;

        return bitcoin::compact::encode(bn_new);
    }

    uint32_t get_next_work_required(const block& prev_block, const uint32_t block_timestamp, GetAncestor get_ancestor,
                                    const bitcoin::core::Params& params) {
        uint32_t pow_limit = params.pow_limit;

        // Only change once per difficulty adjustment interval
        if ((prev_block.height + 1) % params.difficulty_adjustment_interval() != 0) {
            if (params.pow_allow_min_difficulty_blocks) {
                // Special difficulty rule for testnet:
                // If the new block's timestamp is more than 2* 10 minutes
                // then allow mining of a min-difficulty block.
                if (block_timestamp > prev_block.timestamp + params.pow_target_spacing * 2)
                    return pow_limit;
                else {
                    // Return the last non-special-min-difficulty-rules-block
                    block pindex = prev_block;
                    std::optional<block> pprev;
                    bool condition;
                    do {
                        pprev = get_ancestor(pindex.height - 1, pindex.previous_block_hash);
                        condition = pprev.has_value() && pindex.height % params.difficulty_adjustment_interval() != 0
                                    && pindex.bits == pow_limit;
                        pindex = *pprev;
                    } while (condition);

                    return pindex.bits;
                }
            }
            return prev_block.bits;
        }

        // Go back by what we want to be 14 days worth of blocks
        int height_first = prev_block.height - (params.difficulty_adjustment_interval() - 1);
        eosio::check(height_first >= 0, "invalid height_first");
        auto block_first = get_ancestor(height_first, std::nullopt);
        eosio::check(block_first.has_value(), "block first does not exists");

        return calculate_next_work_required(prev_block, block_first->timestamp, get_ancestor, params);
    }

}  // namespace bitcoin::core