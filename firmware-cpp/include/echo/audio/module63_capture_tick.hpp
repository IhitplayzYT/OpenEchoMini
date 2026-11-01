#pragma once

#include "echo/audio/module63_compaction.hpp"

#include <cstdint>
#include <vector>

namespace echo::audio {

enum class Module63CaptureAction : std::uint8_t {
    stage_pending_input,
    wait_for_pending_input,
    append_pending_input,
    submit_sample_buffer,
    lock_capture_event,
    request_slot_transfer,
    unlock_capture_event,
};

struct Module63CaptureTickInput {
    bool state_flag_0c_bit1{};
    bool state_flag_0a_bit1{};
    std::uint8_t state_byte_2{};
    std::uint8_t state_byte_3{};
    std::uint32_t ring_bytes_used{};
    std::uint32_t pending_input_bytes{};
    bool pending_transfer_completes{};
    std::uint32_t processing_state{};
    std::uint32_t layout_mode{};
    std::uint32_t format_marker{};
    std::uint32_t sample_count{};
    std::uint32_t transfer_state_34{};
};

struct Module63CaptureActionRecord {
    Module63CaptureAction action{};
    std::uint32_t value{};
};

struct Module63CaptureTickPlan {
    std::vector<Module63CaptureActionRecord> actions;
    Module63CompactionOutcome compaction_outcome{
        Module63CompactionOutcome::unchanged};
    std::uint32_t transfer_value{};
    std::uint32_t callback_count{};
    bool completion_marker_cleared{true};
    bool state_byte_2_written{};
    std::uint8_t state_byte_2_after{};
    bool state_byte_3_written{};
    std::uint8_t state_byte_3_after{};
    bool work_field_30_cleared{};
    bool transfer_state_34_written{};
    std::uint32_t transfer_state_34_after{};
};

// Plans FUN_03080b02's readiness, pending-input, submit, and service tail.
// Buffer transformation itself is performed separately by
// compact_module63_capture().
[[nodiscard]] Module63CaptureTickPlan plan_module63_capture_tick(
    const Module63CaptureTickInput& input);

} // namespace echo::audio
