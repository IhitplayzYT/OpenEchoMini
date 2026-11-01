#include "echo/audio/module73_seek_stage.hpp"

namespace echo::audio {

Module73SeekStagePlan plan_module73_seek_stage(
    const Module73SeekStageInput& input) {
    Module73SeekStagePlan plan{};
    if (input.mode == 0) {
        plan.actions.push_back({Module73SeekStageAction::record_zero_mode_seek,
                                {input.argument1, input.argument2, 0}, 1});
        return plan;
    }

    if (input.previous_seek_busy) {
        const std::uint64_t maximum_delays = input.timeout_poll_count == 0
            ? (std::uint64_t{1} << 32) + 1u
            : static_cast<std::uint64_t>(input.timeout_poll_count) + 1u;
        const auto needed = input.delay_calls_until_idle == 0
            ? std::uint64_t{1} : input.delay_calls_until_idle;
        const auto delay_count = needed < maximum_delays ? needed : maximum_delays;
        plan.actions.push_back({Module73SeekStageAction::wait_for_previous_seek,
                                {1, 0, 0}, delay_count});
        plan.previous_seek_timed_out = needed >= maximum_delays;
        if (plan.previous_seek_timed_out)
            plan.actions.push_back({Module73SeekStageAction::log_previous_seek_timeout,
                                    {0x01002930u, 0, 0}, 1});
    }

    plan.stream_index = static_cast<std::uint8_t>(input.mode);
    plan.actions.push_back({Module73SeekStageAction::invoke_file_seek,
                            {input.argument1, input.argument2,
                             plan.stream_index}, 1});
    return plan;
}

} // namespace echo::audio
