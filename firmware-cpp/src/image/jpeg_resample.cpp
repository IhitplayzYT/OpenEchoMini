#include "echo/image/jpeg_resample.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace echo::image {
namespace {

constexpr std::uint32_t one_16_16 = 0x10000U;

std::int32_t arithmetic_shift_right(const std::int32_t value,
                                    const unsigned shift) noexcept {
    if (value >= 0) return value >> shift;
    const auto magnitude = -static_cast<std::int64_t>(value);
    return static_cast<std::int32_t>(-((magnitude + ((std::int64_t{1} << shift) - 1)) >> shift));
}

std::uint8_t clamp_byte(const std::int32_t value) noexcept {
    return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

std::uint32_t multiply_16_16(const std::uint32_t left,
                             const std::uint32_t right) noexcept {
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(left) * right) >> 16U);
}

std::uint8_t interpolate_channel(const std::array<Rgb8, 4>& neighbors,
                                 const std::array<std::uint32_t, 4>& weights,
                                 const std::uint8_t Rgb8::*channel) noexcept {
    std::uint32_t weighted_sum = 0;
    for (std::size_t i = 0; i < neighbors.size(); ++i) {
        weighted_sum += static_cast<std::uint32_t>(neighbors[i].*channel) * weights[i];
    }
    const auto scaled = static_cast<float>(weighted_sum) / 65536.0F;
    return static_cast<std::uint8_t>(scaled);
}

} // namespace

Rgb8 module19_ycbcr_to_rgb(const std::uint8_t y_byte,
                           const std::uint8_t cb_byte,
                           const std::uint8_t cr_byte) noexcept {
    const auto y = static_cast<std::int32_t>(y_byte) - 0x10;
    const auto cb = static_cast<std::int32_t>(cb_byte) - 0x80;
    const auto cr = static_cast<std::int32_t>(cr_byte) - 0x80;
    const auto red = arithmetic_shift_right(cr * 0x331 + y * 0x254, 9U) + 4;
    const auto green = arithmetic_shift_right(y * 0x254 - cr * 0x1a0 - cb * 200, 9U) + 4;
    const auto blue = arithmetic_shift_right(cb * 0x409 + y * 0x254, 9U) + 4;
    return Rgb8{clamp_byte(red), clamp_byte(green), clamp_byte(blue)};
}

std::optional<std::uint16_t> module19_bilinear_rgb565(
    const std::array<Rgb8, 4>& neighbors,
    const std::uint32_t fraction_x_16_16,
    const std::uint32_t fraction_y_16_16) noexcept {
    if (fraction_x_16_16 > one_16_16 || fraction_y_16_16 > one_16_16) {
        return std::nullopt;
    }
    const auto x0 = one_16_16 - fraction_x_16_16;
    const auto y0 = one_16_16 - fraction_y_16_16;
    const std::array<std::uint32_t, 4> weights{
        multiply_16_16(x0, y0),
        multiply_16_16(fraction_x_16_16, y0),
        multiply_16_16(x0, fraction_y_16_16),
        multiply_16_16(fraction_x_16_16, fraction_y_16_16),
    };
    const auto red = interpolate_channel(neighbors, weights, &Rgb8::red);
    const auto green = interpolate_channel(neighbors, weights, &Rgb8::green);
    const auto blue = interpolate_channel(neighbors, weights, &Rgb8::blue);
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(red & 0xf8U) << 8U)
        | (static_cast<std::uint16_t>(green & 0xfcU) << 3U)
        | (static_cast<std::uint16_t>(blue) >> 3U));
}

} // namespace echo::image
