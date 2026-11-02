#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule0ExfatDirectoryEntryBytes = 0x20;
inline constexpr std::size_t kModule0ExfatDirectoryChecksumOffset = 2;
inline constexpr std::size_t kModule0ExfatDirectoryMaximumEntryCount = 18;

// Reproduces the checksum write observed in FUN_0305cbe0. The input must be
// one complete 0x85 primary entry followed by its declared secondary entries.
// Bytes 2 and 3 (the checksum field) are skipped during calculation and then
// replaced with the little-endian result. Invalid or truncated sets are left
// unchanged.
[[nodiscard]] bool write_module0_exfat_directory_set_checksum(
    std::span<std::uint8_t> entry_set) noexcept;

} // namespace echo::storage
