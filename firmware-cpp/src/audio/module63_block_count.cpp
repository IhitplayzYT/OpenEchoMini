#include "echo/audio/module63_block_count.hpp"

namespace echo::audio {

Module63BlockCountPlan plan_module63_block_count(
    const Module63BlockCountInput& input) noexcept {
    Module63BlockCountPlan plan{};
    plan.requested_count_written = true;
    plan.requested_count_after = input.requested_count;
    if (input.requested_count == 0) {
        plan.outcome = Module63BlockCountOutcome::reject_zero_count;
        plan.error_callback_requested = true;
        plan.error_code = 0x70;
        return plan;
    }

    plan.sentinel_written = true;
    plan.sentinel_after = 0xffffffffu;
    plan.numerator = input.mode == 2
        ? input.requested_count - 1u
        : input.requested_count * 8u - 8u;
    if (input.unit_size == 0) {
        // The firmware's divisor is state supplied. Avoid host UB for malformed
        // reconstructed input; zero is not a recovered firmware branch.
        plan.outcome = Module63BlockCountOutcome::invalid_zero_divisor;
        return plan;
    }

    plan.outcome = Module63BlockCountOutcome::quotient_ready;
    plan.block_count_written = true;
    plan.block_count_after = plan.numerator / input.unit_size;
    return plan;
}

} // namespace echo::audio
