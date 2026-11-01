#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct ReservoirCopyResult {
    std::size_t blocks_copied{};
    std::uint8_t next_slot{};
};

// Copies complete 0x80-byte blocks from module 15's 16-slot reservoir ring.
// The returned slot can be persisted by the caller for the next operation.
[[nodiscard]] std::optional<ReservoirCopyResult> copy_mpeg_reservoir_blocks(
    std::span<const std::uint8_t> reservoir,
    std::uint8_t first_slot,
    std::size_t requested_blocks,
    std::span<std::uint8_t> output) noexcept;

// Produces the contiguous main-data window addressed by main_data_begin.
// previous_main_data is the caller-maintained history; output must not overlap
// either input span. The returned view is [last back-pointer bytes, current].
[[nodiscard]] std::optional<std::span<const std::uint8_t>>
resolve_mpeg_layer3_main_data_window(
    std::span<const std::uint8_t> previous_main_data,
    std::span<const std::uint8_t> current_main_data,
    std::uint16_t main_data_begin,
    std::span<std::uint8_t> output) noexcept;

// Retains the newest caller-selected number of bytes from old history plus
// this frame's main-data payload for use by the next frame.
[[nodiscard]] std::optional<std::span<const std::uint8_t>>
update_mpeg_layer3_reservoir_tail(
    std::span<const std::uint8_t> previous_main_data,
    std::span<const std::uint8_t> current_main_data,
    std::span<std::uint8_t> output) noexcept;

} // namespace echo::audio
