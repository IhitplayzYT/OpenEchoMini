#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::image {

struct Rgb8 {
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
};

// Packs module 20's per-pixel output callback as RGB565.
[[nodiscard]] std::uint16_t pack_module20_rgb565(Rgb8 pixel) noexcept;

// Generic converter selection by source bit depth and BMP compression value.
// These combinations are observed in module 20's dispatcher; the header path
// allocates a palette only for the uncompressed 1/4/8 bpp modes.
[[nodiscard]] std::optional<std::uint8_t> source_pixel_mode_for(
    std::uint16_t bits_per_pixel,
    std::uint32_t compression) noexcept;

// Pixel conversion cases observed in module 20's generic row converter.
// Palette rows are four-byte entries in B,G,R,unused order.
[[nodiscard]] std::optional<Rgb8> decode_source_pixel(
    std::uint8_t mode,
    std::span<const std::uint8_t> source,
    std::size_t pixel_index,
    std::span<const std::uint8_t> palette = {}) noexcept;

} // namespace echo::image
