#include "echo/display/module0_framebuffer.hpp"

#include <algorithm>
#include <limits>

namespace echo::display {

std::optional<Module0FramebufferPixel> module0_framebuffer_pixel(
    const std::uint16_t x,
    const std::uint16_t y,
    const std::uint16_t bank) noexcept {
    constexpr std::size_t width = 320;
    constexpr std::size_t height = 170;
    constexpr std::size_t split_row = 140;
    constexpr std::size_t row_bytes = width * 2U;
    constexpr std::size_t upper_bank_bytes = split_row * row_bytes;
    constexpr std::size_t lower_bank_bytes = (height - split_row) * row_bytes;

    if (x >= width || y >= height) return std::nullopt;

    const auto column_offset = static_cast<std::size_t>(x) * 2U;
    if (y < split_row) {
        if (bank > std::numeric_limits<std::size_t>::max() / upper_bank_bytes) {
            return std::nullopt;
        }
        return Module0FramebufferPixel{
            Module0FramebufferRegion::upper_rows,
            static_cast<std::size_t>(bank) * upper_bank_bytes
                + static_cast<std::size_t>(y) * row_bytes + column_offset};
    }
    if (bank > std::numeric_limits<std::size_t>::max() / lower_bank_bytes) {
        return std::nullopt;
    }
    return Module0FramebufferPixel{
        Module0FramebufferRegion::lower_rows,
        static_cast<std::size_t>(bank) * lower_bank_bytes
            + (static_cast<std::size_t>(y) - split_row) * row_bytes
            + column_offset};
}

bool write_module0_framebuffer_pixel(
    const std::span<std::uint8_t> upper_rows,
    const std::span<std::uint8_t> lower_rows,
    const std::uint16_t x,
    const std::uint16_t y,
    const std::uint16_t bank,
    const std::uint16_t color) noexcept {
    const auto pixel = module0_framebuffer_pixel(x, y, bank);
    if (!pixel) return false;

    const auto destination = pixel->region == Module0FramebufferRegion::upper_rows
        ? upper_rows : lower_rows;
    if (pixel->byte_offset > destination.size()
        || 2U > destination.size() - pixel->byte_offset) {
        return false;
    }
    destination[pixel->byte_offset] = static_cast<std::uint8_t>(color);
    destination[pixel->byte_offset + 1U] = static_cast<std::uint8_t>(color >> 8U);
    return true;
}

bool write_module0_image_rect(
    const std::span<std::uint8_t> upper_rows,
    const std::span<std::uint8_t> lower_rows,
    const std::span<const std::uint16_t> source_pixels,
    const std::uint16_t x,
    const std::uint16_t y,
    const std::uint16_t width,
    const std::uint16_t height,
    const std::uint16_t bank,
    const Module0ImageBlitMode mode,
    const bool replace_key_color) noexcept {
    if (width == 0 || height == 0) return false;
    if (mode != Module0ImageBlitMode::rgb565
        && mode != Module0ImageBlitMode::clipped_rgb565) {
        return false;
    }

    const auto source_count = static_cast<std::size_t>(width) * height;
    if (source_pixels.size() < source_count) return false;

    constexpr std::uint32_t framebuffer_max_x = 319;
    constexpr std::uint32_t framebuffer_max_y = 169;
    constexpr std::uint32_t clipped_max_x = 156;
    constexpr std::uint32_t clipped_max_y = 146;
    const auto max_x = mode == Module0ImageBlitMode::clipped_rgb565
        ? clipped_max_x : framebuffer_max_x;
    const auto max_y = mode == Module0ImageBlitMode::clipped_rgb565
        ? clipped_max_y : framebuffer_max_y;
    const auto right = std::min<std::uint32_t>(
        static_cast<std::uint32_t>(x) + width - 1U, max_x);
    const auto bottom = std::min<std::uint32_t>(
        static_cast<std::uint32_t>(y) + height - 1U, max_y);
    if (x > right || y > bottom) return true;

    for (std::uint32_t row = y; row <= bottom; ++row) {
        for (std::uint32_t column = x; column <= right; ++column) {
            const auto source_index = static_cast<std::size_t>(row - y) * width
                + static_cast<std::size_t>(column - x);
            auto color = source_pixels[source_index];
            if (replace_key_color && color == 0x18c3U) {
                color = 99;
            }
            if (!write_module0_framebuffer_pixel(
                    upper_rows, lower_rows,
                    static_cast<std::uint16_t>(column),
                    static_cast<std::uint16_t>(row), bank,
                    color)) {
                return false;
            }
        }
    }
    return true;
}

bool write_module0_mono_mask(
    const std::span<std::uint8_t> upper_rows,
    const std::span<std::uint8_t> lower_rows,
    const std::span<const std::uint16_t> row_masks,
    const std::uint16_t x,
    const std::uint16_t y,
    const std::uint16_t width,
    const std::uint16_t height,
    const std::uint16_t bank,
    const std::uint16_t set_color,
    const std::uint16_t clear_color,
    const std::uint8_t draw_flags) noexcept {
    if (width == 0 || height == 0 || row_masks.size() < height) {
        return false;
    }

    const bool transparent_zero = (draw_flags & 0x01U) != 0;
    const bool invert = (draw_flags & 0x02U) != 0;
    for (std::uint32_t row = 0; row < height; ++row) {
        for (std::uint32_t column = 0; column < width; ++column) {
            const bool set = column < 16
                && (row_masks[row] & (0x8000U >> column)) != 0;
            if (!set && !invert && transparent_zero) continue;
            const auto color = set != invert ? set_color : clear_color;
            const auto destination_x = static_cast<std::uint32_t>(x) + column;
            const auto destination_y = static_cast<std::uint32_t>(y) + row;
            if (destination_x >= 320 || destination_y >= 170) continue;
            if (!write_module0_framebuffer_pixel(
                    upper_rows, lower_rows,
                    static_cast<std::uint16_t>(destination_x),
                    static_cast<std::uint16_t>(destination_y), bank, color)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace echo::display
