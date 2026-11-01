#include "echo/audio/module63_finalize.hpp"

#include <algorithm>

namespace echo::audio {

Module63FinalizePlan plan_module63_finalize(
    const Module63FinalizeInput& input) {
    Module63FinalizePlan plan{};
    if (input.service_state != 1) return plan;

    plan.service_state_written = true;
    plan.service_state_after = 2;
    const auto busy = std::min(input.busy_responses_before_clear, 2000u);
    plan.poll_iterations = busy;
    plan.poll_limit_reached = busy == 2000;
    plan.status_reads = plan.poll_limit_reached ? 2000u : busy + 1u;
    plan.actions.push_back(Module63FinalizeAction::poll_external_status);
    if (busy != 0)
        plan.actions.push_back(Module63FinalizeAction::service_wait_iteration);
    plan.actions.push_back(Module63FinalizeAction::set_capture_mode);
    plan.actions.push_back(Module63FinalizeAction::configure_capture_service);
    plan.actions.push_back(Module63FinalizeAction::stop_recording_path);

    plan.flags_0c_written = true;
    plan.flags_0c_after = static_cast<std::uint16_t>(
        input.state_flags_0c & 0xfffdu);
    plan.field_30_cleared = true;
    plan.field_34_cleared = true;
    return plan;
}

} // namespace echo::audio
