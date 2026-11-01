#pragma once

#include <cstdint>
#include <span>

namespace echo::audio {

struct Module33StreamCursor {
    std::uint32_t cursor{};
    std::uint32_t available_end{};
};

enum class Module33BufferStatus : std::uint8_t {
    no_refill = 0,
    refill = 1,
    status_2 = 2,
};

// FUN_0308b2e0's exact status predicate over the buffer's cursor/end offsets.
[[nodiscard]] Module33BufferStatus module33_buffer_status(
    const Module33StreamCursor& state,
    std::uint32_t requested_bytes) noexcept;

// Adapter boundary for the indirect read callback used by FUN_0308b288. It
// receives the recovered append offset, cursor value and opaque runtime
// handle; it returns the callback result used to update available_end.
using Module33RefillCallback = std::uint32_t (*)(
    std::span<std::uint8_t> buffer,
    std::uint32_t append_offset,
    std::uint32_t cursor_value,
    void* runtime_context) noexcept;

// Models buffer compaction and the callback result arithmetic from
// FUN_0308b288. Invalid offsets or an absent callback return false.
[[nodiscard]] bool refill_module33_stream_buffer(
    std::span<std::uint8_t> buffer,
    Module33StreamCursor& state,
    Module33RefillCallback refill,
    void* runtime_context) noexcept;

// Models FUN_0308b312's 0x100-byte chunk skipping, status checks, refills and
// final partial advance. The callback supplies the external file/read source.
[[nodiscard]] bool skip_module33_stream_bytes(
    std::span<std::uint8_t> buffer,
    Module33StreamCursor& state,
    std::uint32_t byte_count,
    Module33RefillCallback refill,
    void* runtime_context) noexcept;

} // namespace echo::audio
