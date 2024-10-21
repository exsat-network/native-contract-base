#include <compete.xsat/compete.xsat.hpp>

#ifdef DEBUG
#include "./src/debug.hpp"
#endif

[[eosio::action]]
void compete::setmindonate(const uint16_t min_donate_rate) {
    require_auth(get_self());
    global_row global = _global.get_or_default();
    global.min_donate_rate = min_donate_rate;
    _global.set(global, get_self());
}

[[eosio::action]]
void compete::addround(const uint8_t quota, const optional<time_point_sec> start_time) {
    require_auth(get_self());
    global_row global = _global.get_or_default();
    check(global.total_activations == global.total_quotas, "compete.xsat::addround: current round quota is not full yet");
    global.round_id++;
    global.total_quotas += quota;
    _global.set(global, get_self());

    _round.emplace(get_self(), [&](auto& row) {
        row.round = global.round_id;
        row.quota = quota;
        row.start_time = start_time.has_value() ? *start_time : time_point_sec(current_time_point().sec_since_epoch());
    });
}

[[eosio::action]]
void compete::activate(const name& validator) {
    require_auth(validator);
    global_row global = _global.get_or_default();
    validator_table _validators(ENDORSER_MANAGE_CONTRACT, ENDORSER_MANAGE_CONTRACT.value);
    auto validator_itr = _validators.require_find(validator.value, "compete.xsat::activate: validator does not exists");
    check(validator_itr->qualification.amount >= 10000000000, "compete.xsat::activate: validator qualification has less than 100 BTC staked");
    check(validator_itr->donate_rate > global.min_donate_rate, "compete.xsat::activate: validator donate rate is less than the minimum donate rate");

    auto activation_itr = _activation.find(validator.value);
    check(activation_itr == _activation.end(), "compete.xsat::activate: validator already activated");

    check(global.total_activations < global.total_quotas, "compete.xsat::activate: current round quota is full, please wait for the next round");

    auto round_itr = _round.require_find(global.round_id, "compete.xsat::activate: round does not exists");
    check(round_itr->start_time <= time_point_sec(current_time_point().sec_since_epoch()), "compete.xsat::activate: round has not started yet");

    _activation.emplace(get_self(), [&](auto& row) {
        row.validator = validator;
        row.round = global.round_id;
        row.activation_time = current_time_point();
    });
    global.total_activations++;
    _global.set(global, get_self());
}
