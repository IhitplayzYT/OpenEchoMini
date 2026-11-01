#pragma once

#include "echo/image/bmp_info.hpp"
#include "echo/image/source_pixel.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::image {

struct DecodedBmp {
    std::uint32_t width{};
    std::uint32_t height{};
    // Top-to-bottom rows, converted to RGB before device-specific packing.
    std::vector<Rgb8> pixels;
};

// Decodes source pixels for the BMP combinations routed to module 20's
// observed kernels. Clipping/scaling and the firmware's external packed-pixel
// callback are not part of this API.
[[nodiscard]] std::optional<DecodedBmp> decode_bmp(
    std::span<const std::uint8_t> bytes);

} // namespace echo::image
