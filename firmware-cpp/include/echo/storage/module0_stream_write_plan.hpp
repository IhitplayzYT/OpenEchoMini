#pragma once

#include "echo/storage/module0_directory_handle_close.hpp"

#include <algorithm>
#include <cstdint>
#include <span>

namespace echo::storage {

inline constexpr std::uint32_t kModule0StorageSectorBytes = 0x200;
inline constexpr std::size_t kModule0HandleCurrentClusterOffset = 0x18;
inline constexpr std::size_t kModule0HandlePositionOffset = 0x20;

enum class Module0StreamWriteGate {
    ready,
    inactive_handle,
    empty_write,
    unaligned_offset,
    offset_past_end,
    size_overflow,
};

struct Module0StreamWritePlan {
    Module0StreamWriteGate gate{Module0StreamWriteGate::inactive_handle};
    std::uint32_t offset{};
    std::uint32_t requested_bytes{};
    std::uint32_t current_position{};
    std::uint32_t current_length{};
    std::uint32_t first_cluster{};
    std::uint32_t current_cluster{};
    std::uint8_t stream_flags{};
    std::uint32_t resulting_length_if_complete{};
    bool uses_contiguous_cluster_chain{};

    [[nodiscard]] constexpr bool is_ready() const noexcept {
        return gate == Module0StreamWriteGate::ready;
    }
};

// Models the preconditions and metadata inputs observed in FUN_0305d870.
// The firmware rejects zero-length writes, offsets not aligned to 512 bytes,
// offsets past current EOF, and handles without state bit 1. It then performs
// actual sector I/O, allocating or linking clusters as needed; this planner
// does not emulate those device operations or partial failures.
[[nodiscard]] inline Module0StreamWritePlan plan_module0_stream_write(
    const std::span<const std::uint8_t, kModule0ResourceHandleSlotBytes> slot,
    const std::uint32_t offset,
    const std::uint32_t requested_bytes) noexcept {
    const auto read_u32_le = [&slot](const std::size_t at) noexcept {
        return static_cast<std::uint32_t>(slot[at])
            | (static_cast<std::uint32_t>(slot[at + 1U]) << 8U)
            | (static_cast<std::uint32_t>(slot[at + 2U]) << 16U)
            | (static_cast<std::uint32_t>(slot[at + 3U]) << 24U);
    };

    Module0StreamWritePlan plan{};
    plan.offset = offset;
    plan.requested_bytes = requested_bytes;
    plan.current_length = read_u32_le(kModule0HandleLengthOffset);
    plan.first_cluster = read_u32_le(kModule0HandleFirstClusterOffset);
    plan.current_cluster =
        read_u32_le(kModule0HandleCurrentClusterOffset);
    plan.current_position = read_u32_le(kModule0HandlePositionOffset);
    plan.stream_flags = slot[kModule0HandleStreamFlagsOffset];
    plan.resulting_length_if_complete = plan.current_length;
    plan.uses_contiguous_cluster_chain = (plan.stream_flags & 0x02U) != 0U;

    if ((slot[0] & 0x02U) == 0U) {
        plan.gate = Module0StreamWriteGate::inactive_handle;
    } else if (requested_bytes == 0U) {
        plan.gate = Module0StreamWriteGate::empty_write;
    } else if ((offset & (kModule0StorageSectorBytes - 1U)) != 0U) {
        plan.gate = Module0StreamWriteGate::unaligned_offset;
    } else if (offset > plan.current_length) {
        plan.gate = Module0StreamWriteGate::offset_past_end;
    } else if (static_cast<std::uint64_t>(offset) + requested_bytes
               > 0xffffffffULL) {
        plan.gate = Module0StreamWriteGate::size_overflow;
    } else {
        const auto end = offset + requested_bytes;
        plan.resulting_length_if_complete =
            std::max(plan.current_length, end);
        plan.gate = Module0StreamWriteGate::ready;
    }
    return plan;
}

} // namespace echo::storage
