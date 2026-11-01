#include "echo/image/jpeg_component_transform.hpp"

#include <bit>

namespace echo::image {
namespace {

std::int16_t signed16(const std::uint16_t value) noexcept {
    return std::bit_cast<std::int16_t>(value);
}

std::int32_t add32(const std::int32_t lhs, const std::int32_t rhs) noexcept {
    const auto sum = std::bit_cast<std::uint32_t>(lhs)
        + std::bit_cast<std::uint32_t>(rhs);
    return std::bit_cast<std::int32_t>(sum);
}

std::int32_t sub32(const std::int32_t lhs, const std::int32_t rhs) noexcept {
    const auto difference = std::bit_cast<std::uint32_t>(lhs)
        - std::bit_cast<std::uint32_t>(rhs);
    return std::bit_cast<std::int32_t>(difference);
}

std::int32_t mul32(const std::int32_t lhs, const std::int32_t rhs) noexcept {
    const auto product = static_cast<std::int64_t>(lhs) * rhs;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(product));
}

std::int32_t multiply_add32(const std::int32_t acc,
                            const std::int32_t lhs,
                            const std::int32_t rhs) noexcept {
    return add32(acc, mul32(lhs, rhs));
}

std::uint16_t first_pass_value(const std::int32_t value) noexcept {
    const auto rounded = add32(value, 0x800);
    return static_cast<std::uint16_t>(
        std::bit_cast<std::uint32_t>(rounded) >> 12U);
}

std::uint8_t clamp_sample(const std::int32_t value) noexcept {
    return static_cast<std::uint8_t>(value < 0 ? 0 : value > 255 ? 255 : value);
}

std::int32_t even_pair(const std::int32_t c2, const std::int32_t c6) noexcept {
    return add32(mul32(c2, 0x3b21), mul32(c6, -0x187e));
}

std::int32_t odd_pair_a(const std::int32_t c1,
                        const std::int32_t c3,
                        const std::int32_t c5,
                        const std::int32_t c7) noexcept {
    auto value = mul32(c1, 0x21f9);
    value = multiply_add32(value, c3, -0x4587);
    value = multiply_add32(value, c5, 0x2e75);
    return multiply_add32(value, c7, -0x6c2);
}

std::int32_t odd_pair_b(const std::int32_t c1,
                        const std::int32_t c3,
                        const std::int32_t c5,
                        const std::int32_t c7) noexcept {
    auto value = mul32(c1, 0x5203);
    value = multiply_add32(value, c3, 0x1ccd);
    value = multiply_add32(value, c5, -0x133e);
    return multiply_add32(value, c7, -0x1050);
}

std::int32_t dc_base(const std::uint16_t value) noexcept {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value) << 14U);
}

std::uint8_t final_sample(const std::int32_t value) noexcept {
    const auto rounded = add32(value, 0x40000);
    const auto shifted = rounded >> 19U;
    return clamp_sample(shifted + 0x80);
}

} // namespace

JpegReducedSampleBlock inverse_jpeg_component_block_4x4(
    const JpegCoefficientBlock& coefficients) noexcept {
    auto work = coefficients;

    // The firmware transforms each row in place, storing its first four
    // intermediate samples in coefficient columns zero through three.
    for (std::size_t row = 0; row < 8; ++row) {
        const auto base = row * 8U;
        const auto c0 = work[base];
        const auto c1 = signed16(work[base + 1U]);
        const auto c2 = signed16(work[base + 2U]);
        const auto c3 = signed16(work[base + 3U]);
        const auto c4 = signed16(work[base + 4U]);
        const auto c5 = signed16(work[base + 5U]);
        const auto c6 = signed16(work[base + 6U]);
        const auto c7 = signed16(work[base + 7U]);

        if (c1 == 0 && c2 == 0 && c3 == 0 && c4 == 0
            && c5 == 0 && c6 == 0 && c7 == 0) {
            const auto value = static_cast<std::uint16_t>(
                static_cast<std::uint32_t>(c0) << 2U);
            for (std::size_t column = 0; column < 4; ++column) {
                work[base + column] = value;
            }
            continue;
        }

        const auto even = even_pair(c2, c6);
        const auto positive = add32(dc_base(c0), even);
        const auto negative = sub32(dc_base(c0), even);
        const auto odd_a = odd_pair_a(c1, c3, c5, c7);
        const auto odd_b = odd_pair_b(c1, c3, c5, c7);
        work[base] = first_pass_value(add32(positive, odd_b));
        work[base + 3U] = first_pass_value(sub32(positive, odd_b));
        work[base + 1U] = first_pass_value(add32(negative, odd_a));
        work[base + 2U] = first_pass_value(sub32(negative, odd_a));
    }

    JpegReducedSampleBlock output{};
    for (std::size_t column = 0; column < 4; ++column) {
        const auto c0 = signed16(work[column]);
        const auto c1 = signed16(work[8U + column]);
        const auto c2 = signed16(work[16U + column]);
        const auto c3 = signed16(work[24U + column]);
        const auto c5 = signed16(work[40U + column]);
        const auto c6 = signed16(work[48U + column]);
        const auto c7 = signed16(work[56U + column]);

        const auto has_ac = c1 != 0 || c2 != 0 || c3 != 0
            || c5 != 0 || c6 != 0 || c7 != 0;
        if (!has_ac) {
            const auto value = clamp_sample(
                ((static_cast<std::int32_t>(c0) + 0x10) >> 5U) + 0x80);
            for (std::size_t row = 0; row < 4; ++row) {
                output[row * 4U + column] = value;
            }
            continue;
        }

        const auto even = even_pair(c2, c6);
        const auto positive = add32(static_cast<std::int32_t>(c0) * 0x4000, even);
        const auto negative = sub32(static_cast<std::int32_t>(c0) * 0x4000, even);
        const auto odd_a = odd_pair_a(c1, c3, c5, c7);
        const auto odd_b = odd_pair_b(c1, c3, c5, c7);
        output[column] = final_sample(add32(positive, odd_b));
        output[12U + column] = final_sample(sub32(positive, odd_b));
        output[4U + column] = final_sample(add32(negative, odd_a));
        output[8U + column] = final_sample(sub32(negative, odd_a));
    }
    return output;
}

} // namespace echo::image
