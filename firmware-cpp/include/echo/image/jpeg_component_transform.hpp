#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::image {

using JpegCoefficientBlock = std::array<std::uint16_t, 64>;
using JpegReducedSampleBlock = std::array<std::uint8_t, 16>;

// Applies module 19's observed fixed-point block transform. It reads an 8x8
// coefficient block and returns the 4x4 sample region written by the routine.
// The full JPEG scan, component placement, and color conversion are separate.
[[nodiscard]] JpegReducedSampleBlock inverse_jpeg_component_block_4x4(
    const JpegCoefficientBlock& coefficients) noexcept;

} // namespace echo::image
