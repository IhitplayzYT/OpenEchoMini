#include "echo/audio/module33_stream_buffer.hpp"

#include <cstring>

namespace echo::audio {

Module33BufferStatus module33_buffer_status(
    const Module33StreamCursor& state,
    const std::uint32_t requested_bytes) noexcept {
    if (state.available_end < requested_bytes) {
        return Module33BufferStatus::no_refill;
    }
    const auto remaining = static_cast<std::int64_t>(state.available_end) -
                           static_cast<std::int64_t>(state.cursor);
    if (remaining < static_cast<std::int64_t>(requested_bytes)) {
        return state.available_end >= 0x100 ? Module33BufferStatus::refill
                                            : Module33BufferStatus::status_2;
    }
    return Module33BufferStatus::no_refill;
}

bool refill_module33_stream_buffer(
    const std::span<std::uint8_t> buffer,
    Module33StreamCursor& state,
    const Module33RefillCallback refill,
    void* const runtime_context) noexcept {
    if (refill == nullptr || state.cursor > state.available_end ||
        state.available_end > buffer.size()) {
        return false;
    }

    if (state.cursor > 0x100) {
        state.cursor = 0;
    } else if (state.cursor != 0) {
        const auto unread = state.available_end - state.cursor;
        std::memmove(buffer.data(), buffer.data() + state.cursor, unread);
    }

    const auto callback_cursor = state.cursor;
    const auto append_offset = state.available_end - callback_cursor;
    if (append_offset > buffer.size()) {
        return false;
    }
    const auto callback_result = refill(buffer, append_offset, callback_cursor,
                                        runtime_context);
    if (callback_cursor != callback_result) {
        const auto new_end = static_cast<std::uint64_t>(callback_result) +
                             state.available_end - callback_cursor;
        if (new_end > buffer.size()) {
            return false;
        }
        state.available_end = static_cast<std::uint32_t>(new_end);
    }
    state.cursor = 0;
    return true;
}

bool skip_module33_stream_bytes(
    const std::span<std::uint8_t> buffer,
    Module33StreamCursor& state,
    std::uint32_t byte_count,
    const Module33RefillCallback refill,
    void* const runtime_context) noexcept {
    while (byte_count >= 0x100) {
        const auto remaining = static_cast<std::int64_t>(state.available_end) -
                               static_cast<std::int64_t>(state.cursor);
        if (state.available_end >= 0x100 && remaining < 0x100) {
            if (!refill_module33_stream_buffer(buffer, state, refill, runtime_context)) {
                return false;
            }
        }
        state.cursor += 0x100;
        byte_count -= 0x100;
    }

    if (byte_count != 0) {
        if (module33_buffer_status(state, byte_count) == Module33BufferStatus::refill &&
            !refill_module33_stream_buffer(buffer, state, refill, runtime_context)) {
            return false;
        }
        state.cursor += byte_count;
    }
    return true;
}

} // namespace echo::audio
