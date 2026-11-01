#include "echo/audio/module80_ape_mono_filter.hpp"

#include <bit>
#include <cstddef>

namespace echo::audio {
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

std::int32_t wrap_mla(std::int32_t accumulator,
                      std::int32_t left,
                      std::int32_t right) noexcept {
    return wrap_add(accumulator, wrap_mul(left, right));
}

std::int32_t reverse_sign(std::int32_t value) noexcept {
    return value > 0 ? -1 : value < 0 ? 1 : 0;
}

} // namespace

std::int32_t module80_ape_mono_post_filter_sample(
    Module80ApeMonoPostFilterState& state,
    std::int32_t decoded_sample) noexcept {
    auto& history = state.history;
    history[50] = state.predictor_value;
    history[49] = wrap_sub(state.predictor_value, history[49]);

    auto accumulator = wrap_mul(history[50], state.coefficients[0]);
    accumulator = wrap_mla(accumulator, history[49], state.coefficients[1]);
    accumulator = wrap_mla(accumulator, history[48], state.coefficients[2]);
    accumulator = wrap_mla(accumulator, history[47], state.coefficients[3]);
    state.predictor_value = wrap_add(decoded_sample, accumulator >> 10);

    history[18] = reverse_sign(history[50]);
    history[17] = reverse_sign(history[49]);
    const auto residual_sign = reverse_sign(decoded_sample);
    const auto output = wrap_add(
        state.predictor_value, wrap_mul(state.average, 0x1f) >> 5);

    state.coefficients[0] = wrap_mla(
        state.coefficients[0], history[18], residual_sign);
    state.coefficients[1] = wrap_mla(
        state.coefficients[1], history[17], residual_sign);
    state.coefficients[2] = wrap_mla(
        state.coefficients[2], history[16], residual_sign);
    state.coefficients[3] = wrap_mla(
        state.coefficients[3], history[15], residual_sign);
    state.average = output;
    for (std::size_t i = 0; i < 50; ++i) history[i] = history[i + 1];
    return output;
}

void module80_ape_mono_post_filter_chunk(
    Module80ApeMonoPostFilterState& state,
    std::span<std::int32_t> samples) noexcept {
    for (auto& sample : samples) {
        sample = module80_ape_mono_post_filter_sample(state, sample);
    }
}

} // namespace echo::audio
