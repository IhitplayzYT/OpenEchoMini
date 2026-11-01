#pragma once

#include <cstdint>

namespace echo::runtime {

struct FirmwareUnsignedDivision {
    std::uint64_t quotient{};
    std::uint64_t remainder{};
};

// Reconstructs the quotient and residual value left by module 0's restoring
// 64-bit unsigned division helper. Division by zero yields UINT64_MAX and
// leaves the numerator as the remainder, matching the firmware loop.
FirmwareUnsignedDivision firmware_unsigned_divmod(
    std::uint64_t numerator, std::uint64_t denominator);

// Reconstructs module 0's restoring 64-bit unsigned division helper. Division
// by zero follows the firmware loop and returns UINT64_MAX.
std::uint64_t firmware_unsigned_divide(std::uint64_t numerator,
                                      std::uint64_t denominator);

} // namespace echo::runtime
