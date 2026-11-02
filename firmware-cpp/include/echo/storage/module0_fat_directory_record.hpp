#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule0FatDirectoryRecordBytes = 0x20U;
inline constexpr std::size_t kModule0FatDirectoryAttributesOffset = 0x0bU;

// Reproduces FUN_03058d7e's observed end-marker normalization in-place.
// Returns true when the four-byte all-0xff prefix was converted to a NUL
// first byte. This applies only when the backend selector is not 3.
[[nodiscard]] bool normalize_module0_fat_directory_end_marker(
    std::span<std::uint8_t, kModule0FatDirectoryRecordBytes> entry,
    std::uint8_t backend_selector) noexcept;

} // namespace echo::storage
