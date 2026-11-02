#pragma once

#include "echo/audio/vorbis_floor0.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {
namespace detail {

[[nodiscard]] inline double vorbis_floor0_bark(double frequency) noexcept {
    const auto squared = frequency * frequency;
    return 13.1 * std::atan(0.00074 * frequency) +
           2.24 * std::atan(0.0000000185 * squared) + 0.0001 * frequency;
}

[[nodiscard]] inline double vorbis_floor0_db_gain(double db) noexcept {
    return std::pow(10.0, db / 20.0);
}

} // namespace detail

// Applies a decoded Floor-0 packet to one half-block of residue spectrum.
// LSP posts use the Q24 representation emitted by the module-77 codebook path.
// This follows the Floor-0 bark/LSP equations; the firmware's fixed-point
// lookup-table rounding is not reproduced, so output is algorithm-equivalent,
// not bit-identical. The firmware renderer scales its input bins after >> 6;
// this model preserves that observable unit convention.
[[nodiscard]] inline std::optional<std::vector<std::int32_t>>
render_vorbis_floor0_spectrum(std::span<const std::int32_t> residue_spectrum,
                              const VorbisFloor0Setup& setup,
                              const VorbisFloor0Packet& floor) {
    constexpr std::size_t maximum_order = 255;
    constexpr std::size_t maximum_bins = 1U << 16;
    constexpr double q24_scale = 16777216.0;

    if (setup.order == 0 || setup.order > maximum_order || setup.rate == 0 ||
        setup.bark_map_size == 0 || setup.amplitude_bits == 0 ||
        setup.amplitude_bits > 32 || residue_spectrum.empty() ||
        residue_spectrum.size() > maximum_bins || floor.amplitude == 0 ||
        !floor.present || floor.lsp_values.size() != setup.order) {
        if (floor.present || residue_spectrum.size() > maximum_bins ||
            residue_spectrum.empty()) {
            return std::nullopt;
        }
        return std::vector<std::int32_t>(residue_spectrum.size(), 0);
    }

    const auto amplitude_max = (std::uint64_t{1} << setup.amplitude_bits) - 1U;
    if (floor.amplitude > amplitude_max) {
        return std::nullopt;
    }

    std::vector<double> lsp;
    lsp.reserve(floor.lsp_values.size());
    for (const auto value : floor.lsp_values) {
        const auto radians = static_cast<double>(value) / q24_scale;
        if (!std::isfinite(radians) || radians < 0.0 || radians > 3.14159265358979323846) {
            return std::nullopt;
        }
        lsp.push_back(2.0 * std::cos(radians));
    }

    const auto bin_count = residue_spectrum.size();
    const auto bark_max = detail::vorbis_floor0_bark(static_cast<double>(setup.rate) / 2.0);
    if (!std::isfinite(bark_max) || bark_max <= 0.0) return std::nullopt;
    std::vector<std::uint16_t> bark_map(bin_count);
    for (std::size_t i = 0; i < bin_count; ++i) {
        const auto frequency = (static_cast<double>(setup.rate) / 2.0) *
                               static_cast<double>(i) / static_cast<double>(bin_count);
        const auto scaled = detail::vorbis_floor0_bark(frequency) *
                            static_cast<double>(setup.bark_map_size) / bark_max;
        const auto index = static_cast<std::size_t>(std::max(0.0, std::floor(scaled)));
        bark_map[i] = static_cast<std::uint16_t>(
            std::min<std::size_t>(index, setup.bark_map_size - 1U));
    }

    // FUN_010059d4 passes the packet decoder's precomputed gain word directly
    // as the renderer amplitude term. The packet helper stores it in sixteenths.
    const auto amplitude_db = static_cast<double>(floor.gain_numerator) / 16.0;
    std::vector<std::int32_t> rendered(residue_spectrum.size());
    const auto pi = 3.14159265358979323846;
    const auto frequency_step = pi / static_cast<double>(setup.bark_map_size);

    std::size_t first_bin = 0;
    while (first_bin < bin_count) {
        const auto bark = bark_map[first_bin];
        const auto w = 2.0 * std::cos(frequency_step * bark);
        double p = 0.5;
        double q = 0.5;
        for (std::size_t j = 0; j + 1U < lsp.size(); j += 2U) {
            q *= w - lsp[j];
            p *= w - lsp[j + 1U];
        }
        if ((lsp.size() & 1U) != 0) {
            q *= lsp.back();
            p *= p * (4.0 - w * w);
            q *= q;
        } else {
            p *= p * (2.0 - w);
            q *= q * (2.0 + w);
        }
        const auto denominator = std::sqrt(std::max(0.0, p + q));
        if (!std::isfinite(denominator) || denominator <= 0.0) return std::nullopt;
        const auto db = amplitude_db / denominator -
                        static_cast<double>(setup.amplitude_offset);
        const auto gain = detail::vorbis_floor0_db_gain(db);
        if (!std::isfinite(gain) || gain < 0.0) return std::nullopt;

        auto end_bin = first_bin + 1U;
        while (end_bin < bin_count && bark_map[end_bin] == bark) ++end_bin;
        for (auto i = first_bin; i < end_bin; ++i) {
            const auto source = static_cast<double>(residue_spectrum[i] >> 6);
            const auto scaled = std::round(source * gain);
            if (!std::isfinite(scaled) ||
                scaled < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
                scaled > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
                return std::nullopt;
            }
            rendered[i] = static_cast<std::int32_t>(scaled);
        }
        first_bin = end_bin;
    }
    return rendered;
}

} // namespace echo::audio
