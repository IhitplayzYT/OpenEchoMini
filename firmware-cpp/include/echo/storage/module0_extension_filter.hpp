#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace echo::storage {

// Mirrors FUN_0305af88's wildcard, three-byte group scan, and flag veto.
// The filter span is bounded for host safety; a NUL first byte ends the group
// list just as it does in the firmware's packed table.
[[nodiscard]] bool module0_extension_filter_accepts(
    std::string_view extension, std::span<const char> packed_filter,
    std::uint8_t file_flags) noexcept;

} // namespace echo::storage
