#include "echo/audio/module73_layer3_requantize.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace echo::audio {

namespace {

constexpr std::array<std::uint32_t, 8869> kPackedMagnitudeTable{
#include "module73_layer3_magnitude_table.inc"
};

// Values at module data address 0x0102ad38, indexed for exponents -3..+3.
constexpr std::array<std::int32_t, 7> kExponentFactors{
    0x983, 0xb50, 0xd74, 0x1000, 0x1306, 0x16b0, 0x1ae8};

constexpr std::uint32_t kMagnitudeTableSplit = 0x180f;
constexpr std::size_t kHighMagnitudeTableCount = 2710;

} // namespace

std::optional<std::uint32_t> module73_layer3_requantize(
    const std::uint32_t quantized_magnitude,
    const std::int32_t exponent) noexcept {
    const std::size_t table_index = quantized_magnitude > 0x180eU
        ? static_cast<std::size_t>(quantized_magnitude - kMagnitudeTableSplit)
        : kHighMagnitudeTableCount + quantized_magnitude;
    if (table_index >= kPackedMagnitudeTable.size()) return std::nullopt;

    const std::uint32_t packed = kPackedMagnitudeTable[table_index];
    std::int64_t scaled_exponent = static_cast<std::int64_t>(exponent / 4) +
        static_cast<std::int64_t>(packed >> 27U);
    std::uint32_t magnitude = packed & 0x07ffffffU;

    if (scaled_exponent < 0) {
        const std::uint64_t right_shift = static_cast<std::uint64_t>(-scaled_exponent);
        if (right_shift >= 32U) {
            magnitude = 0;
        } else {
            const auto shift = static_cast<unsigned>(right_shift);
            const std::uint32_t rounded = (std::uint32_t{1} << shift) + magnitude;
            magnitude = static_cast<std::uint32_t>(
                std::bit_cast<std::int32_t>(rounded) >> shift);
        }
    } else if (scaled_exponent < 5) {
        magnitude <<= static_cast<unsigned>(scaled_exponent);
    } else {
        magnitude = 0x7fffffffU;
    }

    const std::int32_t remainder = exponent % 4;
    if (remainder == 0) return magnitude;
    const auto factor_index = static_cast<std::size_t>(remainder + 3);
    const std::int64_t product =
        static_cast<std::int64_t>(static_cast<std::int32_t>(magnitude)) *
        kExponentFactors[factor_index] + 0x800;
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 12U);
}

} // namespace echo::audio
