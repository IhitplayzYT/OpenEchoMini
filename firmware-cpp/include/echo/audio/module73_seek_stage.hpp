#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace echo::audio {

enum class Module73SeekStageAction : std::uint8_t {
    record_zero_mode_seek,
    wait_for_previous_seek,
    log_previous_seek_timeout,
    invoke_file_seek,
};

struct Module73SeekStageActionRecord {
    Module73SeekStageAction action{};
    std::array<std::uint32_t, 3> arguments{};
    std::uint64_t repeat_count{1};
};

struct Module73SeekStageInput {
    std::uint32_t argument1{};
    std::uint32_t argument2{};
    std::int32_t mode{};
    bool previous_seek_busy{};
    std::uint64_t delay_calls_until_idle{};
    std::uint32_t timeout_poll_count{};
};

struct Module73SeekStagePlan {
    std::vector<Module73SeekStageActionRecord> actions;
    bool previous_seek_timed_out{};
    std::uint8_t stream_index{};
};

// Plans FUN_010027d6's pre-seek wait and file-service dispatch.
[[nodiscard]] Module73SeekStagePlan plan_module73_seek_stage(
    const Module73SeekStageInput& input);

} // namespace echo::audio
