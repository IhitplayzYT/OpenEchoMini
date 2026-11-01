#include "echo/ebook/text_layout.hpp"

#include <algorithm>

namespace echo::ebook {

namespace {

bool in_range(const std::uint32_t value, const std::uint32_t first,
              const std::uint32_t last) noexcept {
    return value >= first && value <= last;
}

bool is_module0_script_class(const std::uint32_t codepoint) noexcept {
    return in_range(codepoint, 0x600, 0x6ff) ||
           in_range(codepoint, 0xa00, 0xa7f) ||
           in_range(codepoint, 0x1980, 0x19df) ||
           in_range(codepoint, 0x1a00, 0x1aff);
}

bool is_module0_hebrew_class(const std::uint32_t codepoint) noexcept {
    return in_range(codepoint, 0x5b0, 0x5b9) ||
           in_range(codepoint, 0x5bb, 0x5c3) ||
           in_range(codepoint, 0x5d0, 0x5ea) ||
           in_range(codepoint, 0x5f0, 0x5f4);
}

} // namespace

std::uint32_t classify_module30_text_unit(const std::uint32_t codepoint) noexcept {
    if (in_range(codepoint, 0x30, 0x39) ||
        in_range(codepoint, 0x61, 0x7a) ||
        in_range(codepoint, 0x41, 0x5a)) {
        return 1;
    }

    if (in_range(codepoint, 0xc0, 0xd6) ||
        in_range(codepoint, 0xd8, 0xf6) ||
        in_range(codepoint, 0xf8, 0xff) ||
        in_range(codepoint, 0x100, 0x1bf) ||
        in_range(codepoint, 0x1c4, 0x241) ||
        in_range(codepoint, 0x401, 0x45f) ||
        codepoint == 0x490 || codepoint == 0x491 ||
        (in_range(codepoint, 0x386, 0x3ff) && codepoint != 0x387) ||
        is_module0_script_class(codepoint) ||
        is_module0_hebrew_class(codepoint)) {
        return 2;
    }

    if (codepoint < 0x100 && codepoint != 0x20 && codepoint != 0x2c &&
        codepoint != 0x2e && codepoint != 0x2d) {
        return 0;
    }

    return 0x8000;
}

std::size_t count_display_units(
    const std::span<const std::uint8_t> nul_terminated_text,
    const std::uint8_t encoding_mode,
    const std::size_t maximum_source_characters) noexcept {
    const bool two_byte_mode = encoding_mode < 2 || encoding_mode == 3 || encoding_mode == 4;
    std::size_t byte_offset = 0;
    std::size_t source_characters = 0;
    std::size_t display_units = 0;

    while (byte_offset < nul_terminated_text.size() &&
           nul_terminated_text[byte_offset] != 0 &&
           source_characters < maximum_source_characters) {
        const auto first = nul_terminated_text[byte_offset];
        if (two_byte_mode && first >= 0x81 && first <= 0xfe) {
            byte_offset += 2;
            display_units += 2;
        } else {
            ++byte_offset;
            ++display_units;
        }
        ++source_characters;
    }

    return display_units;
}

WrappedLine wrap_line(
    const std::span<const std::uint8_t> input,
    const std::size_t pixel_limit,
    const CharacterByteLength& byte_length,
    const CharacterPixelWidth& pixel_width,
    const BreakAllowedBefore& break_allowed_before) {
    if (input.empty() || !byte_length || !pixel_width) {
        return {};
    }

    std::size_t offset = 0;
    std::size_t pixels = 0;
    std::size_t last_break = 0;
    std::size_t pixels_at_break = 0;
    while (offset < input.size()) {
        const auto remaining = input.subspan(offset);
        const auto first = remaining.front();
        if (first == '\r' || first == '\n') {
            const auto newline_bytes = first == '\r' && remaining.size() > 1 && remaining[1] == '\n'
                ? 2U : 1U;
            return {offset + newline_bytes, pixels, true};
        }

        auto bytes = byte_length(remaining);
        if (bytes == 0) {
            bytes = 1;
        }
        bytes = std::min(bytes, remaining.size());
        const auto width = pixel_width(remaining.first(bytes));
        if (width > pixel_limit - std::min(pixel_limit, pixels)) {
            if (last_break != 0) {
                return {last_break, pixels_at_break, false};
            }
            if (offset == 0) {
                return {bytes, width, false};
            }
            return {offset, pixels, false};
        }

        if (break_allowed_before && break_allowed_before(remaining.first(bytes))) {
            last_break = offset;
            pixels_at_break = pixels;
        }
        pixels += width;
        offset += bytes;
    }
    return {offset, pixels, false};
}

PageStart rewind_page_start(
    const std::size_t current_byte_offset,
    const std::span<const std::size_t> recent_line_byte_lengths,
    const std::size_t requested_lines) noexcept {
    const auto count = std::min(requested_lines, recent_line_byte_lengths.size());
    std::size_t offset = current_byte_offset;
    std::size_t rewound = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const auto length = recent_line_byte_lengths[recent_line_byte_lengths.size() - 1 - i];
        if (length > offset) {
            offset = 0;
            break;
        }
        offset -= length;
        ++rewound;
    }
    return {offset, rewound};
}

std::array<char16_t, 11> format_page_indicator(
    const std::uint32_t current,
    const std::uint32_t total) noexcept {
    constexpr std::uint32_t overflow_threshold = 100000;
    std::array<char16_t, 11> result{};
    const auto write_number = [&result, overflow_threshold](
                                 const std::uint32_t value,
                                 const std::size_t offset) {
        result[offset] = value < overflow_threshold
            ? static_cast<char16_t>(u'0' + value / 10000U)
            : u'M';
        result[offset + 1] = static_cast<char16_t>(u'0' + (value / 1000U) % 10U);
        result[offset + 2] = static_cast<char16_t>(u'0' + (value % 1000U) / 100U);
        result[offset + 3] = static_cast<char16_t>(u'0' + (value / 10U) % 10U);
        result[offset + 4] = static_cast<char16_t>(u'0' + value % 10U);
    };

    write_number(current, 0);
    result[5] = u'/';
    write_number(total, 6);
    return result;
}

} // namespace echo::ebook
