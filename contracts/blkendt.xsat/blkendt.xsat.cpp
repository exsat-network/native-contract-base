#include <blkendt.xsat/blkendt.xsat.hpp>
#include <utxomng.xsat/utxomng.xsat.hpp>
#include <endrmng.xsat/endrmng.xsat.hpp>
#include <rescmng.xsat/rescmng.xsat.hpp>
#include "../internal/safemath.hpp"

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

//@auth utxomng.xsat
[[eosio::action]]
void block_endorse::erase(const uint64_t height) {
    require_auth(UTXO_MANAGE_CONTRACT);

    block_endorse::endorsement_table _endorsement(get_self(), height);
    auto endorsement_itr = _endorsement.begin();
    while (endorsement_itr != _endorsement.end()) {
        endorsement_itr = _endorsement.erase(endorsement_itr);
    }
}

//@auth get_self()
[[eosio::action]]
void block_endorse::config(const uint64_t limit_endorse_height) {
    require_auth(get_self());
    auto config = _config.get_or_default();
    config.limit_endorse_height = limit_endorse_height;
    _config.set(config, get_self());
}

//@auth validator
[[eosio::action]]
void block_endorse::endorse(const name& validator, const uint64_t height, const checksum256& hash) {
    require_auth(validator);

    auto config = _config.get_or_default();
    check(config.limit_endorse_height == 0 || config.limit_endorse_height >= height,
          "blkendt.xsat::endorse: the current endorsement status is disabled");

    utxo_manage::chain_state_table _chain_state(UTXO_MANAGE_CONTRACT, UTXO_MANAGE_CONTRACT.value);
    auto chain_state = _chain_state.get();
    check(chain_state.irreversible_height < height && chain_state.migrating_height != height,
          "blkendt.xsat::endorse: the block has been parsed and does not need to be endorsed");

    // fee deduction
    resource_management::pay_action pay(RESOURCE_MANAGE_CONTRACT, {get_self(), "active"_n});
    pay.send(height, hash, validator, ENDORSE, 1);

    block_endorse::endorsement_table _endorsement(get_self(), height);
    auto endorsement_idx = _endorsement.get_index<"byhash"_n>();
    auto endorsement_itr = endorsement_idx.find(hash);
    bool reached_consensus = false;
    if (endorsement_itr == endorsement_idx.end()) {
        std::vector<requested_validator_info> requested_validators = get_valid_validator();
        check(!requested_validators.empty(),
              "blkendt.xsat::endorse: no validators found with staking amounts exceeding 100 BTC");
        auto itr = std::find_if(requested_validators.begin(), requested_validators.end(),
                                [&](const requested_validator_info& a) {
                                    return a.account == validator;
                                });
        check(itr != requested_validators.end(), "blkendt.xsat::endorse: the validator has less than 100 BTC staked");
        provider_validator_info provider_info{
            .account = itr->account, .staking = itr->staking, .created_at = current_time_point()};
        requested_validators.erase(itr);
        auto endt_itr = _endorsement.emplace(get_self(), [&](auto& row) {
            row.id = _endorsement.available_primary_key();
            row.hash = hash;
            row.provider_validators.push_back(provider_info);
            row.requested_validators = requested_validators;
        });
        reached_consensus = endt_itr->num_reached_consensus() <= endt_itr->provider_validators.size();
    } else {
        check(std::find_if(endorsement_itr->provider_validators.begin(), endorsement_itr->provider_validators.end(),
                           [&](const provider_validator_info& a) {
                               return a.account == validator;
                           })
                  == endorsement_itr->provider_validators.end(),
              "blkendt.xsat::endorse: validator is on the list of provider validators");

        auto itr = std::find_if(endorsement_itr->requested_validators.begin(),
                                endorsement_itr->requested_validators.end(), [&](const requested_validator_info& a) {
                                    return a.account == validator;
                                });
        check(itr != endorsement_itr->requested_validators.end(),
              "blkendt.xsat::endorse: the validator has less than 100 BTC staked");
        endorsement_idx.modify(endorsement_itr, same_payer, [&](auto& row) {
            row.provider_validators.push_back(
                {.account = itr->account, .staking = itr->staking, .created_at = current_time_point()});
            row.requested_validators.erase(itr);
        });
        reached_consensus = endorsement_itr->num_reached_consensus() <= endorsement_itr->provider_validators.size();
    }

    if (reached_consensus) {
            utxo_manage::consensus_action _consensus(UTXO_MANAGE_CONTRACT, {get_self(), "active"_n});
            _consensus.send(height, hash);
    }
}

std::vector<block_endorse::requested_validator_info> block_endorse::get_valid_validator() {
    endorse_manage::validator_table _validator
        = endorse_manage::validator_table(ENDORSER_MANAGE_CONTRACT, ENDORSER_MANAGE_CONTRACT.value);
    auto idx = _validator.get_index<"bystaked"_n>();
    auto itr = idx.lower_bound(MIN_STAKE_FOR_ENDORSEMENT);
    std::vector<requested_validator_info> result;
    while (itr != idx.end()) {
        result.emplace_back(
            requested_validator_info{.account = itr->owner, .staking = static_cast<uint64_t>(itr->quantity.amount)});
        itr++;
    }
    return result;
}
