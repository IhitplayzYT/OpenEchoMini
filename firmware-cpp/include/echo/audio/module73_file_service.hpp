#pragma once

#include <cstdint>
#include <vector>

namespace echo::audio {

enum class Module73FileOperation : std::uint8_t { seek, read };
enum class Module73FileAction : std::uint8_t {
    arm_event,
    submit_request,
    delay_one_tick,
    log_timeout,
    store_stream_result,
    clear_completion_flag,
};

struct Module73FileActionRecord {
    Module73FileAction action{};
    std::uint32_t first{};
    std::uint32_t second{};
    std::uint32_t third{};
    std::uint64_t repeat_count{1};
};

struct Module73FileRequestInput {
    Module73FileOperation operation{};
    std::uint32_t argument1{};
    std::uint32_t argument2{};
    std::uint8_t stream_index{};
    bool read_guard_passes{true};
    bool completion_ready_on_entry{};
    bool completion_never_arrives{};
    // Number of delay(1) calls required before completion is observed.
    std::uint32_t delay_calls_until_completion{};
    std::uint32_t timeout_poll_count{};
    std::uint32_t returned_status{};
};

struct Module73FileRequestPlan {
    std::vector<Module73FileActionRecord> actions;
    bool read_guard_rejected{};
    bool timed_out{};
    bool stream_result_written{};
    std::uint32_t stream_result{};
    bool completion_flag_cleared{};
    std::uint32_t return_value{};
};

// Plans module 73's event 0x69 seek and event 0x6b read request paths.
[[nodiscard]] Module73FileRequestPlan plan_module73_file_request(
    const Module73FileRequestInput& input);

} // namespace echo::audio
