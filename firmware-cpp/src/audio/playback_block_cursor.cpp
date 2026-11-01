#include "echo/audio/playback_block_cursor.hpp"

namespace echo::audio {

PlaybackCursorStep advance_playback_block_cursor(
    PlaybackBlockCursor& cursor, PlaybackBlockRefill refill,
    void* context) noexcept {
    cursor.position = static_cast<std::uint16_t>(cursor.position + 1u);
    if (cursor.available_bytes > cursor.position) {
        return PlaybackCursorStep::advanced;
    }

    constexpr std::uint16_t end_flag = 0x0001u;
    if ((cursor.flags & end_flag) != 0) {
        return PlaybackCursorStep::end_of_stream;
    }
    if (refill == nullptr) return PlaybackCursorStep::refill_unavailable;

    constexpr std::uint16_t block_size = 0x0800u;
    cursor.available_bytes = refill(context);
    if (cursor.available_bytes < block_size) cursor.flags |= end_flag;
    cursor.position = 0;
    return PlaybackCursorStep::advanced;
}

} // namespace echo::audio
