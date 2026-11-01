#pragma once

#include <array>
#include <cstdint>

namespace echo::runtime {

struct Module0SampleMuxState {
    std::array<std::uint16_t, 8> samples{};
    std::array<std::uint8_t, 32> queued_channels{};
    std::uint8_t current_channel{};
    std::uint8_t queue_cursor{};
    std::uint32_t pending_flags{};
    std::uint32_t queued_count{};
};

struct Module0SampleMuxInterruptPlan {
    Module0SampleMuxState state{};
    std::uint32_t acknowledge_status_mask{0x40U};
    bool stored_sample{};
    bool consumed_queue_entry{};
    bool reconfigured_hardware{};
    std::uint8_t next_channel{};
    std::uint32_t hardware_control_clear_mask{};
    std::uint32_t hardware_control_set_mask{};
};

// Models the module 0 vector 36 halfword-sample path. Peripheral access is
// returned as data; this function performs no MMIO.
[[nodiscard]] Module0SampleMuxInterruptPlan module0_sample_mux_interrupt(
    Module0SampleMuxState state, std::uint16_t sample) noexcept;

} // namespace echo::runtime
