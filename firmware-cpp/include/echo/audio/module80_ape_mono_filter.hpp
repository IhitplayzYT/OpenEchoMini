#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace echo::audio {

struct Module80ApeMonoPostFilterState {
    std::int32_t predictor_value{};
    std::int32_t average{};
    std::array<std::int32_t, 4> coefficients{};
    std::array<std::int32_t, 51> history{};
};

// Reconstructs FUN_01003340's single-channel adaptive post-filter.
[[nodiscard]] std::int32_t module80_ape_mono_post_filter_sample(
    Module80ApeMonoPostFilterState& state,
    std::int32_t decoded_sample) noexcept;

void module80_ape_mono_post_filter_chunk(
    Module80ApeMonoPostFilterState& state,
    std::span<std::int32_t> samples) noexcept;

} // namespace echo::audio
