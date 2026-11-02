#pragma once

#include "echo/storage/module0_resource_handle_slots.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule0HandleLengthOffset = 0x10;
inline constexpr std::size_t kModule0HandleFirstClusterOffset = 0x14;
inline constexpr std::size_t kModule0HandleDirectoryEntryOffset = 0x1c;
inline constexpr std::size_t kModule0HandleStreamFlagsOffset = 0x260;

struct Module0Mode3CloseUpdate {
    std::uint32_t directory_entry_index{};
    std::uint32_t file_length{};
    std::uint32_t first_cluster{};
    std::uint8_t stream_flags{};
    bool set_archive_attribute{true};
    bool release_slot_after_update_call{true};
};

// Decodes only fields read by FUN_03059c74 when closing a mode-3 slot whose
// state has bit 1 set. The close path writes file_length into both the
// stream's valid-data and data-length fields, writes first_cluster, sets the
// primary archive attribute, recalculates the entry-set checksum, then frees
// the slot after the write call. It does not model sector I/O or errors.
[[nodiscard]] inline std::optional<Module0Mode3CloseUpdate>
decode_module0_mode3_close_update(
    const std::span<const std::uint8_t, kModule0ResourceHandleSlotBytes> slot)
    noexcept {
    if ((slot[0] & 0x02U) == 0U) {
        return std::nullopt;
    }

    const auto read_u32_le = [&slot](const std::size_t offset) noexcept {
        return static_cast<std::uint32_t>(slot[offset])
            | (static_cast<std::uint32_t>(slot[offset + 1U]) << 8U)
            | (static_cast<std::uint32_t>(slot[offset + 2U]) << 16U)
            | (static_cast<std::uint32_t>(slot[offset + 3U]) << 24U);
    };

    Module0Mode3CloseUpdate result{};
    result.file_length = read_u32_le(kModule0HandleLengthOffset);
    result.first_cluster = read_u32_le(kModule0HandleFirstClusterOffset);
    result.directory_entry_index =
        read_u32_le(kModule0HandleDirectoryEntryOffset);
    result.stream_flags = slot[kModule0HandleStreamFlagsOffset];
    return result;
}

} // namespace echo::storage
