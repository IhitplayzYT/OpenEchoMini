#include "echo/image/source_pixel.hpp"

#include <limits>

namespace echo::image {
namespace {

std::optional<Rgb8> palette_pixel(const std::span<const std::uint8_t> palette,
                                  const std::size_t index) noexcept {
    if (index > (std::numeric_limits<std::size_t>::max() - 2U) / 4U) {
        return std::nullopt;
    }
    const auto offset = index * 4U;
    if (offset + 2U >= palette.size()) {
        return std::nullopt;
    }
    return Rgb8{palette[offset + 2U], palette[offset + 1U], palette[offset]};
}

} // namespace

std::uint16_t pack_module20_rgb565(const Rgb8 pixel) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(pixel.red & 0xf8U) << 8U)
        | (static_cast<std::uint16_t>(pixel.green & 0xfcU) << 3U)
        | (static_cast<std::uint16_t>(pixel.blue) >> 3U));
}

std::optional<std::uint8_t> source_pixel_mode_for(
    const std::uint16_t bits_per_pixel,
    const std::uint32_t compression) noexcept {
    if (compression == 0U) {
        switch (bits_per_pixel) {
        case 1: return 0;
        case 4: return 1;
        case 8: return 2;
        case 16: return 3;
        case 24: return 4;
        case 32: return 5;
        default: return std::nullopt;
        }
    }
    if (compression == 3U) {
        if (bits_per_pixel == 16U) return 8;
        if (bits_per_pixel == 32U) return 9;
    }
    return std::nullopt;
}

std::optional<Rgb8> decode_source_pixel(
    const std::uint8_t mode,
    const std::span<const std::uint8_t> source,
    const std::size_t pixel_index,
    const std::span<const std::uint8_t> palette) noexcept {
    switch (mode) {
    case 0: {
        const auto byte_index = pixel_index / 8U;
        if (byte_index >= source.size()) return std::nullopt;
        const auto shift = static_cast<unsigned>(7U - (pixel_index % 8U));
        const auto color_index = static_cast<std::size_t>((source[byte_index] >> shift) & 1U);
        return palette_pixel(palette, color_index);
    }
    case 1: {
        const auto byte_index = pixel_index / 2U;
        if (byte_index >= source.size()) return std::nullopt;
        const auto packed = source[byte_index];
        const auto color_index = static_cast<std::size_t>(
            (pixel_index % 2U == 0U) ? (packed >> 4U) : (packed & 0x0fU));
        return palette_pixel(palette, color_index);
    }
    case 2:
        if (pixel_index >= source.size()) return std::nullopt;
        return palette_pixel(palette, source[pixel_index]);
    case 3:
    case 8: {
        if (pixel_index > (std::numeric_limits<std::size_t>::max() - 1U) / 2U) {
            return std::nullopt;
        }
        const auto offset = pixel_index * 2U;
        if (offset + 1U >= source.size()) return std::nullopt;
        const auto packed = static_cast<std::uint16_t>(source[offset])
            | static_cast<std::uint16_t>(static_cast<std::uint16_t>(source[offset + 1U]) << 8U);
        if (mode == 3) {
            return Rgb8{static_cast<std::uint8_t>(((packed >> 10U) & 0x1fU) << 3U),
                        static_cast<std::uint8_t>(((packed >> 5U) & 0x1fU) << 3U),
                        static_cast<std::uint8_t>((packed & 0x1fU) << 3U)};
        }
        return Rgb8{static_cast<std::uint8_t>(((packed >> 11U) & 0x1fU) << 3U),
                    static_cast<std::uint8_t>(((packed >> 5U) & 0x3fU) << 2U),
                    static_cast<std::uint8_t>((packed & 0x1fU) << 3U)};
    }
    case 4:
    case 5:
    case 9: {
        const auto stride = mode == 4 ? 3U : 4U;
        const auto first = mode == 9 ? 1U : 0U;
        if (pixel_index > (std::numeric_limits<std::size_t>::max() - first - 2U) / stride) {
            return std::nullopt;
        }
        const auto offset = pixel_index * stride + first;
        if (offset + 2U >= source.size()) return std::nullopt;
        return Rgb8{source[offset + 2U], source[offset + 1U], source[offset]};
    }
    default:
        return std::nullopt;
    }
}

} // namespace echo::image
