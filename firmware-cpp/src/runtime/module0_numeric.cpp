#include "echo/runtime/module0_numeric.hpp"

#include <bit>

namespace echo::runtime {
namespace {

std::uint32_t shift_significand(const std::uint32_t significand,
                                const int shift) noexcept {
    if (shift >= 32 || shift <= -32) return 0;
    if (shift >= 0) return significand << static_cast<unsigned>(shift);
    return significand >> static_cast<unsigned>(-shift);
}

std::uint32_t magnitude_integer(const std::uint32_t bits) noexcept {
    const auto exponent = static_cast<int>((bits >> 23U) & 0xffU);
    if (exponent < 127) return 0;
    const auto significand = (bits & 0x007fffffU) | 0x00800000U;
    return shift_significand(significand, exponent - 150);
}

} // namespace

std::int32_t module0_float32_to_i32(const std::uint32_t bits) noexcept {
    const auto magnitude = magnitude_integer(bits);
    const auto signed_bits = (bits & 0x80000000U) != 0
        ? 0U - magnitude
        : magnitude;
    // The firmware returns the two's-complement bit pattern in r0.
    return std::bit_cast<std::int32_t>(signed_bits);
}

std::uint32_t module0_float32_magnitude_to_u32(
    const std::uint32_t bits) noexcept {
    return magnitude_integer(bits & 0x7fffffffU);
}

std::uint64_t module0_float32_to_float64_bits(
    const std::uint32_t bits) noexcept {
    const auto magnitude = bits & 0x7fffffffU;
    if (magnitude == 0) return 0;

    const auto sign = static_cast<std::uint64_t>(bits & 0x80000000U) << 32U;
    const auto exponent = static_cast<std::uint64_t>((bits >> 23U) & 0xffU) + 0x380U;
    const auto fraction = static_cast<std::uint64_t>(bits & 0x007fffffU);
    return sign | (exponent << 52U) | (fraction << 29U);
}

} // namespace echo::runtime
