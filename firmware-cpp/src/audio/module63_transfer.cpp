#include "echo/audio/module63_transfer.hpp"

namespace echo::audio {

Module63TransferPlan plan_module63_slot_transfer(
    const Module63TransferInput& input) noexcept {
    Module63TransferPlan plan{};
    if (input.transfer_state == 1) {
        plan.outcome = Module63TransferOutcome::clear_stale_transfer;
        plan.state_byte1_written = true;
        plan.state_byte1_after = 1;
        plan.state34_written = true;
        plan.state34_after = 0;
        return plan;
    }
    if (!input.completion_ready) return plan;

    plan.outcome = Module63TransferOutcome::start_alternating_slot;
    plan.previous_buffer_written = true;
    plan.previous_buffer_after = input.current_buffer;
    plan.active_slot_written = true;
    plan.active_slot_after = 1 - input.active_slot;
    plan.current_buffer_written = true;
    plan.current_buffer_after = input.slot_region_base +
        static_cast<std::uint32_t>(plan.active_slot_after) * 0x3000u;
    plan.completion_flag_written = true;
    plan.completion_flag_after = 1;
    plan.transfer_state_written = true;
    plan.transfer_state_after = 1;
    plan.transfer_bytes = input.sample_count >> 1;
    plan.state_flags_after = static_cast<std::uint16_t>(
        (input.state_flags & 0xfffeu) | 0x0002u);
    return plan;
}

} // namespace echo::audio
