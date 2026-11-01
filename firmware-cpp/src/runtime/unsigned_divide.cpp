#include "echo/runtime/unsigned_divide.hpp"

#include <limits>

namespace echo::runtime {

FirmwareUnsignedDivision firmware_unsigned_divmod(
    std::uint64_t numerator, std::uint64_t denominator) {
    std::uint64_t quotient = 0;
    for (int bit = 63; bit >= 0; --bit) {
        if ((numerator >> bit) < denominator) continue;
        numerator -= denominator << bit;
        quotient |= std::uint64_t{1} << bit;
    }
    if (denominator == 0) quotient = std::numeric_limits<std::uint64_t>::max();
    return {quotient, numerator};
}

std::uint64_t firmware_unsigned_divide(std::uint64_t numerator,
                                      std::uint64_t denominator) {
    return firmware_unsigned_divmod(numerator, denominator).quotient;
}

} // namespace echo::runtime
