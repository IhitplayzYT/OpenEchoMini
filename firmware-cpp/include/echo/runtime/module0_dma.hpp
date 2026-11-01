#pragma once

#include <array>
#include <cstdint>

namespace echo::runtime {

enum class Module0DmaPlanError : std::uint8_t {
    none,
    invalid_requested_channel,
    no_available_channel,
    transfer_too_large,
};

struct Module0DmaChunk {
    std::uint32_t offset{};
    std::uint32_t length{};
};

struct Module0DmaPlan {
    static constexpr std::uint32_t channel_count = 6;
    static constexpr std::uint32_t chunk_bytes = 0x0ff8U;
    static constexpr std::uint32_t maximum_chunks = 18;

    Module0DmaPlanError error{Module0DmaPlanError::none};
    std::uint8_t channel{0xffU};
    std::array<Module0DmaChunk, maximum_chunks> chunks{};
    std::uint8_t chunk_count{};
};

// Chooses the requested DMA channel when free. If it is occupied, the
// firmware scans only channels 0 through 4 as fallbacks, then describes the
// observed 0xff8-byte transfer segmentation.
// This planner does not access the controller or construct hardware records.
[[nodiscard]] Module0DmaPlan module0_plan_dma_transfer(
    std::uint8_t requested_channel, std::uint8_t busy_channels,
    std::uint32_t transfer_bytes) noexcept;

} // namespace echo::runtime
