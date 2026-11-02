#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule0DirectoryRecordBytes = 0x20;
inline constexpr std::size_t kModule0QueuedNameUnitsPerRecord = 15;
inline constexpr std::size_t kModule0QueuedNameOutputBytes = 0x100;

struct Module0QueuedWideName {
    std::array<char16_t,
               kModule0QueuedNameOutputBytes / sizeof(char16_t)> code_units{};
    std::uint8_t length{};
};

// Decodes the mode-3 long-name path in FUN_0305bd9e. The marker record must
// begin with 0xc0; its byte at +3 is the UTF-16 code-unit count. Continuation
// records begin at the next supplied record and contribute units at +2..+30.
// Counts above the observed 0x100-byte output capacity are rejected safely.
[[nodiscard]] std::optional<Module0QueuedWideName>
decode_module0_queued_wide_name(
    std::span<const std::uint8_t> marker_record,
    std::span<const std::uint8_t> continuation_records) noexcept;

} // namespace echo::storage
