#include "echo/audio/module80_ape_predictor_step.hpp"
#include "echo/audio/module80_ape_decode_plan.hpp"

#include <algorithm>
#include <bit>
#include <vector>

namespace echo::audio {

static_assert(plan_module80_ape_decode_chunk(0x1001, 2, 0).sample_count == 0x1000);
namespace {

std::int32_t wrap_add(std::int32_t left, std::int32_t right) noexcept {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(left) +
                                      std::bit_cast<std::uint32_t>(right));
}

std::int32_t wrap_sub(std::int32_t left, std::int32_t right) noexcept {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(left) -
                                      std::bit_cast<std::uint32_t>(right));
}

std::int32_t wrap_mul(std::int32_t left, std::int32_t right) noexcept {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(left) *
                                      std::bit_cast<std::uint32_t>(right));
}

std::int32_t wrap_neg(std::int32_t value) noexcept {
    return std::bit_cast<std::int32_t>(std::uint32_t{0} -
                                      std::bit_cast<std::uint32_t>(value));
}

std::int32_t wrap_mla(std::int32_t accumulator,
                      std::int32_t left,
                      std::int32_t right) noexcept {
    return wrap_add(accumulator, wrap_mul(left, right));
}

std::int32_t reverse_sign(std::int32_t value) noexcept {
    return value > 0 ? -1 : value < 0 ? 1 : 0;
}

} // namespace

std::optional<std::int32_t> module80_ape_predictor_step(
    std::span<std::int32_t, kModule80ApePredictorStateWords> state,
    std::span<std::int32_t> rolling_history,
    std::size_t sample_index,
    std::uint32_t channel,
    std::int32_t decoded_residual) noexcept {
    constexpr auto max_offset = std::size_t{50};
    if (channel > 1 || sample_index > rolling_history.size() ||
        rolling_history.size() - sample_index <= max_offset) {
        return std::nullopt;
    }

    const auto offsets = module80_ape_predictor_tap_offsets(channel);
    auto at = [&](std::uint8_t offset) -> std::int32_t& {
        return rolling_history[sample_index + offset];
    };
    auto at_back = [&](std::uint8_t offset, std::size_t back) -> std::int32_t& {
        return rolling_history[sample_index + offset - back];
    };

    const auto channel_word = static_cast<std::size_t>(channel);
    at(offsets.residual) = state[channel_word + 1];
    at(offsets.residual_sign) = reverse_sign(at(offsets.residual));
    at_back(offsets.residual, 1) = wrap_sub(
        at(offsets.residual), at_back(offsets.residual, 1));
    at_back(offsets.residual_sign, 1) = reverse_sign(
        at_back(offsets.residual, 1));

    const auto first_filter = 7 + channel * 4;
    auto first_sum = wrap_mul(at(offsets.residual), state[first_filter]);
    first_sum = wrap_mla(first_sum, at_back(offsets.residual, 1),
                         state[first_filter + 1]);
    first_sum = wrap_mla(first_sum, at_back(offsets.residual, 2),
                         state[first_filter + 2]);
    first_sum = wrap_mla(first_sum, at_back(offsets.residual, 3),
                         state[first_filter + 3]);

    const auto other_channel = channel ^ 1U;
    const auto cross_state = static_cast<std::size_t>(other_channel) + 3;
    const auto cross_decay_state = channel_word + 5;
    const auto cross_decay = wrap_mul(state[cross_decay_state], 0x1f) >> 5;
    at(offsets.cross_channel) = wrap_sub(state[cross_state], cross_decay);
    at(offsets.cross_sign) = reverse_sign(at(offsets.cross_channel));
    at_back(offsets.cross_channel, 1) = wrap_sub(
        at(offsets.cross_channel), at_back(offsets.cross_channel, 1));
    at_back(offsets.cross_sign, 1) = reverse_sign(at_back(offsets.cross_channel, 1));
    state[cross_decay_state] = state[cross_state];

    const auto second_filter = 15 + channel * 5;
    auto second_sum = wrap_mul(at(offsets.cross_channel), state[second_filter]);
    for (std::size_t tap = 1; tap < 5; ++tap) {
        second_sum = wrap_mla(second_sum,
                              at_back(offsets.cross_channel, tap),
                              state[second_filter + tap]);
    }

    auto prediction = wrap_add(first_sum, second_sum >> 1) >> 10;
    const auto output = wrap_add(decoded_residual, prediction);
    state[channel_word + 1] = output;

    const auto smoothed_state = channel_word + 3;
    const auto smoothed_decay = wrap_mul(state[smoothed_state], 0x1f) >> 5;
    state[smoothed_state] = wrap_add(output, smoothed_decay);

    const auto coefficient_sign = reverse_sign(decoded_residual);
    const auto first_sign_index = static_cast<std::size_t>(offsets.residual_sign);
    for (std::size_t tap = 0; tap < 4; ++tap) {
        state[first_filter + tap] = wrap_mla(
            state[first_filter + tap],
            rolling_history[sample_index + first_sign_index - tap],
            coefficient_sign);
    }
    const auto second_sign_index = static_cast<std::size_t>(offsets.cross_sign);
    for (std::size_t tap = 0; tap < 5; ++tap) {
        state[second_filter + tap] = wrap_mla(
            state[second_filter + tap],
            rolling_history[sample_index + second_sign_index - tap],
            coefficient_sign);
    }
    return state[smoothed_state];
}

bool module80_ape_predictor_chunk(
    std::span<std::int32_t, kModule80ApePredictorStateWords> state,
    std::span<std::int32_t> channel_zero,
    std::span<std::int32_t> channel_one) {
    if (channel_zero.size() != channel_one.size() || channel_zero.size() > 0x1000) {
        return false;
    }

    const auto count = channel_zero.size();
    std::vector<std::int32_t> rolling(count + kModule80ApePredictorStateWords);
    std::copy(state.begin(), state.end(), rolling.begin());
    for (std::size_t i = 0; i < count; ++i) {
        const auto zero = module80_ape_predictor_step(
            state, rolling, i, 0, channel_zero[i]);
        const auto one = module80_ape_predictor_step(
            state, rolling, i, 1, channel_one[i]);
        if (!zero || !one) return false;
        channel_zero[i] = *zero;
        channel_one[i] = *one;
    }

    std::copy(rolling.begin() + static_cast<std::ptrdiff_t>(count),
              rolling.end(), state.begin());

    for (std::size_t i = 0; i < count; ++i) {
        const auto mid = channel_zero[i];
        const auto half_mid = mid < 0 ? wrap_neg(wrap_neg(mid) >> 1) : mid >> 1;
        const auto side_minus_half = wrap_sub(channel_one[i], half_mid);
        channel_zero[i] = side_minus_half;
        channel_one[i] = wrap_add(mid, side_minus_half);
    }
    return true;
}

} // namespace echo::audio
