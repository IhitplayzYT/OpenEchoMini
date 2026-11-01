#pragma once

#include <cstdint>

namespace echo::audio {

struct PlaybackBlockCursor {
    std::uint16_t flags{};
    std::uint16_t available_bytes{};
    std::uint16_t position{};
};

enum class PlaybackCursorStep {
    advanced,
    end_of_stream,
    refill_unavailable,
};

using PlaybackBlockRefill = std::uint16_t (*)(void* context);

// Advances module 27's 16-bit cursor. On boundary it asks the caller to refill
// one 0x800-byte block, records the returned length, marks EOF on a short read,
// and resets position to zero. A zero-length final block is reported as
// advanced once, then end_of_stream on the next boundary attempt, matching the
// firmware helper's update order.
[[nodiscard]] PlaybackCursorStep advance_playback_block_cursor(
    PlaybackBlockCursor& cursor, PlaybackBlockRefill refill,
    void* context) noexcept;

} // namespace echo::audio
