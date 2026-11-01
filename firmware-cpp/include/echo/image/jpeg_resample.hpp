#pragma once

#include "echo/image/source_pixel.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace echo::image {

// Module 19's fixed-coefficient Y, Cb, Cr conversion with byte clamping.
[[nodiscard]] Rgb8 module19_ycbcr_to_rgb(
    std::uint8_t y, std::uint8_t cb, std::uint8_t cr) noexcept;

// Input order: top-left, top-right, bottom-left, bottom-right.
// Fractions are unsigned 16.16 values in the inclusive range [0, 1.0].
[[nodiscard]] std::optional<std::uint16_t> module19_bilinear_rgb565(
    const std::array<Rgb8, 4>& neighbors,
    std::uint32_t fraction_x_16_16,
    std::uint32_t fraction_y_16_16) noexcept;

} // namespace echo::image
