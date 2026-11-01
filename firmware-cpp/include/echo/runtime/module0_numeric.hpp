#pragma once

#include <cstdint>

namespace echo::runtime {

// Module 0 soft-float helpers operate on IEEE-shaped bit patterns, but their
// special-case and rounding behavior is target-specific. These conversions
// model the observed Thumb instructions rather than host casts.
[[nodiscard]] std::int32_t module0_float32_to_i32(std::uint32_t bits) noexcept;
[[nodiscard]] std::uint32_t module0_float32_magnitude_to_u32(
    std::uint32_t bits) noexcept;
[[nodiscard]] std::uint64_t module0_float32_to_float64_bits(
    std::uint32_t bits) noexcept;

} // namespace echo::runtime
