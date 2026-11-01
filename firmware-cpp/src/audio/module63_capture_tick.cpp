#include "echo/audio/module63_capture_tick.hpp"

namespace echo::audio {
namespace {

void add(Module63CaptureTickPlan& plan, Module63CaptureAction action,
         std::uint32_t value = 0) {
    plan.actions.push_back({action, value});
}

} // namespace

Module63CaptureTickPlan plan_module63_capture_tick(
    const Module63CaptureTickInput& input) {
    Module63CaptureTickPlan plan{};
    if (!input.state_flag_0c_bit1 || !input.state_flag_0a_bit1) return plan;

    const std::uint64_t ring_end =
        static_cast<std::uint64_t>(input.ring_bytes_used) + input.pending_input_bytes;
    const bool stage_pending = input.state_byte_2 == 1 &&
        input.state_byte_3 != 0 && ring_end <= 0xc000u;
    if (stage_pending) {
        add(plan, Module63CaptureAction::stage_pending_input,
            (input.pending_input_bytes >> 2) +
                ((input.pending_input_bytes & 3u) != 0));
    }

    plan.transfer_value = input.sample_count << 1;
    const bool valid_processing_state =
        input.processing_state == 1 || input.processing_state == 2;
    if (valid_processing_state && input.layout_mode == 3) {
        if (input.processing_state == 1) {
            plan.transfer_value = (input.sample_count >> 1) << 1;
            plan.compaction_outcome = Module63CompactionOutcome::compact_first_samples;
        } else {
            plan.compaction_outcome = Module63CompactionOutcome::duplicate_first_samples;
        }
    } else if (valid_processing_state && input.layout_mode == 4) {
        plan.compaction_outcome = Module63CompactionOutcome::select_second_samples;
        if (input.processing_state == 1)
            plan.transfer_value = (input.sample_count >> 1) << 1;
    } else if (valid_processing_state) {
        plan.compaction_outcome = Module63CompactionOutcome::invalid_mode_pair;
    }
    plan.callback_count = plan.transfer_value >> 2;
    add(plan, Module63CaptureAction::submit_sample_buffer,
        plan.callback_count);
    plan.state_byte_2_written = true;
    plan.state_byte_2_after = 0;

    if (stage_pending) {
        add(plan, Module63CaptureAction::wait_for_pending_input);
        if (!input.pending_transfer_completes) return plan;
        add(plan, Module63CaptureAction::append_pending_input,
            input.pending_input_bytes);
    }

    if (input.state_byte_3 == 0) {
        plan.state_byte_3_written = true;
        plan.state_byte_3_after = 1;
        plan.work_field_30_cleared = true;
        return plan;
    }

    add(plan, Module63CaptureAction::lock_capture_event, 0x16);
    plan.work_field_30_cleared = true;
    if (input.transfer_state_34 == 0)
        add(plan, Module63CaptureAction::request_slot_transfer);
    plan.transfer_state_34_written = true;
    plan.transfer_state_34_after = 1;
    add(plan, Module63CaptureAction::unlock_capture_event, 0x16);
    return plan;
}

} // namespace echo::audio
