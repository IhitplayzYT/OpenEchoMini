#include "echo/audio/module80_ape_model.hpp"
#include "echo/audio/module80_ape_predictor_seed.hpp"

#include "echo/audio/module80_ape_tables.hpp"

#include <bit>

namespace echo::audio {

static_assert(module80_ape_predictor_state_seed().channel_seed_words[0][0] == 360);

bool update_module80_ape_adaptive_model(
    Module80ApeAdaptiveModelState& state,
    std::uint32_t decoded_unsigned_value) noexcept {
    if (state.k >= kModule80ApeModelBoundaries.size() - 1) return false;

    // The Thumb code computes signed (value + 1) / 2, truncating toward zero,
    // then applies the correction in 32-bit unsigned arithmetic.
    const auto signed_rounded = std::bit_cast<std::int32_t>(
        decoded_unsigned_value + std::uint32_t{1});
    const auto midpoint = signed_rounded / 2;
    const auto decay = (state.average + std::uint32_t{16}) >> 5;
    state.average = state.average + static_cast<std::uint32_t>(midpoint) - decay;

    if (state.average < kModule80ApeModelBoundaries[state.k]) {
        --state.k;
    } else if (kModule80ApeModelBoundaries[state.k + 1] <= state.average) {
        ++state.k;
    }
    return true;
}

} // namespace echo::audio
