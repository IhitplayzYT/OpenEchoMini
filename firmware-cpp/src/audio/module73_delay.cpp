#include "echo/audio/module73_delay.hpp"

namespace echo::audio {

Module73DelayPlan plan_module73_delay(const Module73DelayInput& input) noexcept {
    Module73DelayPlan plan{};
    if (input.clock_denominator == 0) return plan;

    plan.outcome = Module73DelayOutcome::computed;
    plan.divided_clock = input.clock_numerator / input.clock_denominator;
    plan.scaled_ticks = plan.divided_clock * input.requested_ticks;
    const std::uint64_t product =
        static_cast<std::uint64_t>(input.reciprocal_multiplier) *
        plan.scaled_ticks;
    plan.busy_loop_iterations = static_cast<std::uint32_t>(product >> 34);
    return plan;
}

} // namespace echo::audio
