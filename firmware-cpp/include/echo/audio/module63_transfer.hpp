#pragma once

#include <cstdint>

namespace echo::audio {

enum class Module63TransferOutcome : std::uint8_t {
    no_action,
    clear_stale_transfer,
    start_alternating_slot,
};

struct Module63TransferInput {
    std::int32_t transfer_state{};
    bool completion_ready{};
    std::int32_t active_slot{};
    std::uint32_t current_buffer{};
    std::uint32_t slot_region_base{};
    std::uint32_t sample_count{};
    std::uint16_t state_flags{};
};

struct Module63TransferPlan {
    Module63TransferOutcome outcome{Module63TransferOutcome::no_action};
    bool state_byte1_written{};
    std::uint8_t state_byte1_after{};
    bool state34_written{};
    std::uint32_t state34_after{};
    bool previous_buffer_written{};
    std::uint32_t previous_buffer_after{};
    bool active_slot_written{};
    std::int32_t active_slot_after{};
    bool current_buffer_written{};
    std::uint32_t current_buffer_after{};
    bool completion_flag_written{};
    std::int16_t completion_flag_after{};
    bool transfer_state_written{};
    std::int32_t transfer_state_after{};
    std::uint32_t transfer_bytes{};
    std::uint16_t state_flags_after{};
};

// Plans FUN_03080652's stale-transfer reset or alternating 0x3000-byte slot
// submission. It does not invoke the DMA callback.
[[nodiscard]] Module63TransferPlan plan_module63_slot_transfer(
    const Module63TransferInput& input) noexcept;

} // namespace echo::audio
