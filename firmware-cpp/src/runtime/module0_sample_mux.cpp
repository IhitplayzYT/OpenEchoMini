#include "echo/runtime/module0_sample_mux.hpp"

namespace echo::runtime {

Module0SampleMuxInterruptPlan module0_sample_mux_interrupt(
    const Module0SampleMuxState state, const std::uint16_t sample) noexcept {
    Module0SampleMuxInterruptPlan plan{};
    plan.state = state;

    if (state.current_channel < plan.state.samples.size()) {
        plan.state.samples[state.current_channel] = sample;
        plan.stored_sample = true;
        plan.state.pending_flags |= 1U << state.current_channel;
    }
    plan.state.pending_flags |= 0x10000U;

    if (state.queued_count == 0) {
        return plan;
    }

    plan.consumed_queue_entry = true;
    plan.state.queued_count = state.queued_count - 1U;
    const auto cursor = static_cast<std::uint8_t>(state.queue_cursor & 0x1fU);
    const auto next = state.queued_channels[cursor];
    plan.state.queue_cursor = static_cast<std::uint8_t>((cursor + 1U) & 0x1fU);
    if (next >= plan.state.samples.size()) {
        return plan;
    }

    plan.reconfigured_hardware = true;
    plan.next_channel = next;
    plan.hardware_control_clear_mask = 0x17U;
    plan.hardware_control_set_mask = static_cast<std::uint32_t>(next) | 0x10U;
    plan.state.current_channel = next;
    plan.state.pending_flags &= ~0x10000U;
    return plan;
}

} // namespace echo::runtime
