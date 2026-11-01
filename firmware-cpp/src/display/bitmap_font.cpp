#include "echo/display/bitmap_font.hpp"

#include <limits>
#include <stdexcept>

namespace echo::display {

FontTableLayout small_font_layout() noexcept {
    return {0x20, 0x18, 0x18, 0x1a, 12};
}

FontTableLayout large_font_layout() noexcept {
    return {0x21, 0x20, 0x20, 0x22, 16};
}

FontGlyphView read_font_glyph(std::span<const std::uint8_t> table,
                              std::uint32_t codepoint,
                              FontTableLayout layout) {
    if (layout.glyph_stride == 0 || layout.metadata_offset > layout.read_bytes
        || layout.read_bytes - layout.metadata_offset < 2
        || layout.bitmap_bytes > layout.metadata_offset) {
        throw std::invalid_argument("invalid observed font-table layout");
    }
    if (codepoint > std::numeric_limits<std::size_t>::max() / layout.glyph_stride) {
        throw std::out_of_range("font glyph offset overflow");
    }
    const auto offset = static_cast<std::size_t>(codepoint) * layout.glyph_stride;
    if (offset > table.size() || layout.read_bytes > table.size() - offset) {
        throw std::out_of_range("font glyph exceeds supplied table bytes");
    }
    return {
        table.subspan(offset, layout.bitmap_bytes),
        table.subspan(offset + layout.metadata_offset, 2),
        layout.bitmap_height,
    };
}

std::vector<std::uint8_t> expand_glyph_1bpp(
    std::span<const std::uint8_t> bitmap, std::uint8_t width,
    std::uint8_t height) {
    if (width == 0 || width > 16 || height == 0) {
        throw std::invalid_argument("unsupported 1-bpp glyph dimensions");
    }
    const auto required = static_cast<std::size_t>(height) * 2;
    if (bitmap.size() < required) throw std::out_of_range("short 1-bpp glyph bitmap");

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height);
    for (std::size_t row = 0; row < height; ++row) {
        const auto word = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(bitmap[row * 2]) << 8)
            | bitmap[row * 2 + 1]);
        for (std::size_t column = 0; column < width; ++column) {
            pixels[row * width + column] =
                (word & (static_cast<std::uint16_t>(0x8000U >> column))) != 0 ? 1 : 0;
        }
    }
    return pixels;
}

std::vector<std::uint16_t> rasterize_glyph_1bpp(
    std::span<const std::uint8_t> bitmap, std::uint8_t width,
    std::uint8_t height, std::uint16_t foreground,
    std::uint16_t background) {
    const auto mask = expand_glyph_1bpp(bitmap, width, height);
    std::vector<std::uint16_t> pixels;
    pixels.reserve(mask.size());
    for (const auto bit : mask) pixels.push_back(bit != 0 ? foreground : background);
    return pixels;
}

} // namespace echo::display
