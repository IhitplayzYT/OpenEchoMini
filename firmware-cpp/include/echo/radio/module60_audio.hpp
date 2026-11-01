#pragma once

#include <array>
#include <cstdint>

namespace echo::radio {

struct Module60AudioInput {
    std::uint16_t sample_rate_selector{};
    std::uint16_t small_buffer_variant{};
    std::uint8_t dma_buffer_toggle{};
    std::uint32_t stream5_handle{};
    std::uint32_t stream4_handle{};
    std::uint32_t dma_buffer_base{};
};

struct Module60DmaStart {
    std::uint32_t stream_id{};
    std::uint32_t handle{};
    std::uint32_t buffer_address{};
    std::uint32_t byte_count{};
    std::uint32_t callback_address{};
};

struct Module60AudioPlan {
    bool has_sample_rate{};
    std::uint32_t sample_rate_hz{};
    std::uint8_t device_format_code{};
    std::uint32_t callback_buffer_size{};
    std::array<Module60DmaStart, 2> dma_starts{};
};

// Models the selector, buffer sizing, and alternating DMA setup in
// FUN_03093f08/FUN_03094032. Hardware calls remain adapter actions.
[[nodiscard]] Module60AudioPlan plan_module60_audio_setup(
    const Module60AudioInput& input) noexcept;

} // namespace echo::radio
