#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::text {

// Observed sparse byte-pair lookup layouts in firmware modules 67, 69, and
// 70. Each encoded lead byte selects a 256-slot row; the trail byte is mapped
// to the slot offset used by that firmware table.
enum class DoubleByteTableLayout {
    cp932,
    big5,
    cp949,
    cp949_hanja,
};

// Reads one 16-bit little-endian codepoint slot. Zero is the observed empty
// slot value in the extracted mapping tables.
[[nodiscard]] std::optional<std::uint16_t> read_codepoint_table_slot(
    std::span<const std::uint8_t> table,
    std::size_t slot) noexcept;

// Finds the first slot containing a codepoint. The returned number is a table
// slot only; its downstream glyph-index meaning is not yet established.
[[nodiscard]] std::optional<std::size_t> find_codepoint_table_slot(
    std::span<const std::uint8_t> table,
    std::uint16_t codepoint) noexcept;

// Looks up byte values in a 128-entry high-byte mapping table whose first slot
// corresponds to byte 0x80. This covers the observed module 65/66 table shape.
[[nodiscard]] std::optional<std::uint16_t> lookup_high_byte_codepoint(
    std::span<const std::uint8_t> table,
    std::uint8_t value) noexcept;

// Looks up a two-byte character in one of the observed sparse firmware maps.
// The supplied table may be a truncated prefix of the encoding's full grid.
[[nodiscard]] std::optional<std::uint16_t> lookup_double_byte_codepoint(
    std::span<const std::uint8_t> table,
    DoubleByteTableLayout layout,
    std::uint8_t lead,
    std::uint8_t trail) noexcept;

} // namespace echo::text
