#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::audio {

enum class Module33CoefficientSource : std::uint8_t {
    none,
    table_a,
    table_b,
    custom_state,
};

struct Module33CustomCoefficientWrite {
    std::uint8_t input_halfword_index{};
    std::uint8_t state_byte_offset{};
    std::uint16_t additive_bias{};
};

struct Module33RateSelectionPlan {
    std::uint32_t input_rate{};
    std::uint32_t processing_rate{};
    std::uint8_t mode{};
    Module33CoefficientSource source{Module33CoefficientSource::none};
    std::uint16_t table_byte_offset{};
    std::array<Module33CustomCoefficientWrite, 5> custom_writes{};
    bool mode_recognized{};
};

// Models module 33's observed rate substitutions and mode-to-coefficient-row
// selection. The coefficient row contents and the callee that consumes them
// are external to module 33 and are deliberately not reconstructed here.
[[nodiscard]] Module33RateSelectionPlan plan_module33_rate_selection(
    std::uint32_t input_rate,
    std::uint8_t mode) noexcept;

} // namespace echo::audio
