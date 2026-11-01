#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace echo::display {

struct FontTableLayout {
    std::size_t glyph_stride{};
    std::size_t bitmap_bytes{};
    std::size_t metadata_offset{};
    std::size_t read_bytes{};
    std::uint8_t bitmap_height{};
};

struct FontGlyphView {
    std::span<const std::uint8_t> bitmap;
    std::span<const std::uint8_t> metadata;
    std::uint8_t bitmap_height{};
};

// Layouts observed in module 0's text renderer. Width is stored per glyph and
// remains in the raw metadata bytes because its exact encoding is unresolved.
[[nodiscard]] FontTableLayout small_font_layout() noexcept;
[[nodiscard]] FontTableLayout large_font_layout() noexcept;

// Reads one codepoint-indexed glyph from a caller-supplied table byte range.
// The external table base is not included in this span.
[[nodiscard]] FontGlyphView read_font_glyph(
    std::span<const std::uint8_t> table, std::uint32_t codepoint,
    FontTableLayout layout);

// Expands one 16-bit-word-per-row, MSB-first 1-bit glyph into 0/1 pixels.
[[nodiscard]] std::vector<std::uint8_t> expand_glyph_1bpp(
    std::span<const std::uint8_t> bitmap, std::uint8_t width,
    std::uint8_t height);

// Maps the expanded 1-bit mask to the two 16-bit color values used by the
// firmware's pixel helper. Color encoding is intentionally left to the caller.
[[nodiscard]] std::vector<std::uint16_t> rasterize_glyph_1bpp(
    std::span<const std::uint8_t> bitmap, std::uint8_t width,
    std::uint8_t height, std::uint16_t foreground,
    std::uint16_t background);

} // namespace echo::display
