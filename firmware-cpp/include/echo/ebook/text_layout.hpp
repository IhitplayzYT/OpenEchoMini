#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace echo::ebook {

// The firmware's encoding tables and font metrics are not recovered. Callers
// provide the byte width of one source character and its rendered pixel width.
using CharacterByteLength = std::function<std::size_t(std::span<const std::uint8_t>)>;
using CharacterPixelWidth = std::function<std::size_t(std::span<const std::uint8_t>)>;
using BreakAllowedBefore = std::function<bool(std::span<const std::uint8_t>)>;

// Counts the firmware's display units for its four two-byte encoding modes.
// Input follows the firmware contract: a byte sequence terminated by NUL.
[[nodiscard]] std::size_t count_display_units(
    std::span<const std::uint8_t> nul_terminated_text,
    std::uint8_t encoding_mode,
    std::size_t maximum_source_characters) noexcept;

// Returns the module 30 classifier's raw result: 0, 1, 2, or 0x8000. The
// punctuation/script tables are reconstructed from the shared helper ranges.
[[nodiscard]] std::uint32_t classify_module30_text_unit(
    std::uint32_t codepoint) noexcept;

struct WrappedLine {
    std::size_t source_bytes{};
    std::size_t rendered_pixels{};
    bool ended_by_newline{};
};

// Consumes at most one visual line. CR, LF and CRLF terminate a line. If the
// next character would exceed the width, the last permitted break is used;
// absent one, the line ends before that character. A too-wide first character
// is consumed to guarantee progress.
[[nodiscard]] WrappedLine wrap_line(
    std::span<const std::uint8_t> input,
    std::size_t pixel_limit,
    const CharacterByteLength& byte_length,
    const CharacterPixelWidth& pixel_width,
    const BreakAllowedBefore& break_allowed_before = {});

struct PageStart {
    std::size_t byte_offset{};
    std::size_t lines_rewound{};
};

// The reader keeps recent visual-line source lengths to move back from a
// cursor. Lengths are supplied oldest-to-newest and include consumed newline
// bytes when present, matching source offsets rather than display widths.
[[nodiscard]] PageStart rewind_page_start(
    std::size_t current_byte_offset,
    std::span<const std::size_t> recent_line_byte_lengths,
    std::size_t requested_lines) noexcept;

// Formats two page values as five digits each separated by '/'. Values at or
// above 100000 use 'M' in the first position, matching the module 30 constant.
[[nodiscard]] std::array<char16_t, 11> format_page_indicator(
    std::uint32_t current,
    std::uint32_t total) noexcept;

} // namespace echo::ebook
