#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace echo::audio {

// The firmware builds one 256-entry table for every group of eight signed
// coefficients. A set bit selects +coefficient; a clear bit selects
// -coefficient. The firmware stores groups contiguously inside each pattern.
struct DsdDecimatorLookup {
    static constexpr std::size_t firmware_groups_per_pattern = 60;
    std::size_t groups{};
    std::vector<std::int32_t> pattern_group_values;

    [[nodiscard]] std::int32_t at(std::uint8_t byte_pattern, std::size_t group) const {
        if (group >= groups) {
            throw std::out_of_range("DSD lookup group is outside the populated range");
        }
        return pattern_group_values.at(
            static_cast<std::size_t>(byte_pattern) * firmware_groups_per_pattern + group);
    }
};

// Reproduces the observed lookup arithmetic, including ARM's modulo-2^32
// multiply-accumulate result, with owned output and bounded coefficient reads.
[[nodiscard]] inline DsdDecimatorLookup build_dsd_decimator_lookup(
    std::span<const std::int32_t> coefficients) {
    DsdDecimatorLookup result;
    result.groups = (coefficients.size() + 7) / 8;
    if (result.groups > DsdDecimatorLookup::firmware_groups_per_pattern) {
        throw std::length_error("firmware DSD lookup rows hold at most 60 groups");
    }
    result.pattern_group_values.resize(
        256 * DsdDecimatorLookup::firmware_groups_per_pattern);

    for (std::size_t pattern = 0; pattern < 256; ++pattern) {
        for (std::size_t group = 0; group < result.groups; ++group) {
            std::uint32_t sum = 0;
            const std::size_t first = group * 8;
            const std::size_t end = first + 8 < coefficients.size()
                ? first + 8
                : coefficients.size();
            for (std::size_t tap = first; tap < end; ++tap) {
                const bool bit_set = ((pattern >> (tap - first)) & 1U) != 0;
                const auto coefficient = std::bit_cast<std::uint32_t>(coefficients[tap]);
                sum += bit_set ? coefficient : (0U - coefficient);
            }
            result.pattern_group_values[pattern * DsdDecimatorLookup::firmware_groups_per_pattern + group] =
                std::bit_cast<std::int32_t>(sum);
        }
    }
    return result;
}

} // namespace echo::audio
