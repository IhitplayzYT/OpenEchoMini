#pragma once

#include <cstdint>

namespace echo::audio {

enum class Module73DelayOutcome : std::uint8_t {
    computed,
    invalid_zero_divisor,
};

struct Module73DelayInput {
    std::uint32_t clock_numerator{};
    std::uint32_t clock_denominator{};
    std::uint32_t reciprocal_multiplier{};
    std::uint32_t requested_ticks{};
};

struct Module73DelayPlan {
    Module73DelayOutcome outcome{Module73DelayOutcome::invalid_zero_divisor};
    std::uint32_t divided_clock{};
    std::uint32_t scaled_ticks{};
    std::uint32_t busy_loop_iterations{};
};

// Models FUN_01000c08's reciprocal-scaled busy-wait count, without waiting.
[[nodiscard]] Module73DelayPlan plan_module73_delay(
    const Module73DelayInput& input) noexcept;

} // namespace echo::audio
