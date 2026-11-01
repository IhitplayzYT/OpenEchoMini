#include "echo/audio/module73_file_service.hpp"

namespace echo::audio {

Module73FileRequestPlan plan_module73_file_request(
    const Module73FileRequestInput& input) {
    Module73FileRequestPlan plan{};
    const bool is_seek = input.operation == Module73FileOperation::seek;
    const std::uint32_t event = is_seek ? 0x69u : 0x6bu;
    plan.actions.push_back({Module73FileAction::arm_event, event, 0, 2});
    plan.actions.push_back({Module73FileAction::submit_request,
                            input.argument1, input.argument2,
                            input.stream_index});

    if (!is_seek && !input.read_guard_passes) {
        plan.read_guard_rejected = true;
        plan.return_value = 0;
        return plan;
    }

    if (!input.completion_ready_on_entry) {
        const std::uint64_t maximum_delays = input.timeout_poll_count == 0
            ? (std::uint64_t{1} << 32) + 1u
            : static_cast<std::uint64_t>(input.timeout_poll_count) + 1u;
        const auto requested_delays = input.delay_calls_until_completion == 0
            ? std::uint64_t{1} : input.delay_calls_until_completion;
        const auto delay_count = input.completion_never_arrives ||
                requested_delays >= maximum_delays
            ? maximum_delays : requested_delays;
        if (delay_count != 0)
            plan.actions.push_back({Module73FileAction::delay_one_tick,
                                    1, 0, 0, delay_count});
        plan.timed_out = input.completion_never_arrives ||
            requested_delays >= maximum_delays;
        if (plan.timed_out)
            plan.actions.push_back({Module73FileAction::log_timeout,
                                    is_seek ? 0x010004d8u : 0x010004f4u, 0, 0});
    }

    plan.stream_result = is_seek ? input.argument1 : input.argument2;
    plan.actions.push_back({Module73FileAction::store_stream_result,
                            input.stream_index, plan.stream_result, 0});
    plan.stream_result_written = true;
    plan.actions.push_back({Module73FileAction::clear_completion_flag, 0, 0, 0});
    plan.completion_flag_cleared = true;
    plan.return_value = input.returned_status;
    return plan;
}

} // namespace echo::audio
