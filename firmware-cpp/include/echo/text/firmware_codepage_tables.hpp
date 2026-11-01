#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::text {

// The c8 firmware region begins with fourteen 0x20000-byte, 65536-entry
// big-endian input-unit to Unicode tables. Names reflect observed mapping
// matches and are not firmware-provided labels.
enum class FirmwareCodepage : std::uint8_t {
    windows_874,
    windows_932,
    windows_936,
    windows_949,
    windows_950,
    windows_1250,
    windows_1251,
    windows_1252,
    windows_1253,
    windows_1254,
    windows_1255,
    windows_1256,
    windows_1257,
    windows_1258,
};

// Returns nullopt when the selected table or input slot is unavailable, or
// when a nonzero input maps to the table's zero/unmapped sentinel. Input zero
// remains a valid U+0000 mapping.
[[nodiscard]] std::optional<std::uint16_t> lookup_firmware_codepage(
    std::span<const std::uint8_t> region,
    FirmwareCodepage codepage,
    std::uint16_t input_unit) noexcept;

} // namespace echo::text
