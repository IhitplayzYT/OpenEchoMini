#include "echo/audio/module73_layer3_stereo.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace echo::audio {
namespace {

constexpr std::array<std::uint16_t, 7> kMpeg1IntensityGains{
    0U, 865U, 1499U, 2048U, 2596U, 3230U, 4096U};
constexpr std::array<std::array<std::uint16_t, 15>, 2> kLsfIntensityGains{{
    {{3444U, 2896U, 2435U, 2048U, 1722U, 1448U, 1217U, 1024U,
      861U, 724U, 608U, 512U, 430U, 362U, 304U}},
    {{2896U, 2048U, 1448U, 1024U, 724U, 512U, 362U, 256U,
      181U, 128U, 90U, 64U, 45U, 32U, 22U}},
}};
constexpr std::int64_t kMidSideQ12 = 2896;

std::optional<std::int32_t> rounded_q12_product(
    const std::int64_t value, const std::int64_t gain) noexcept {
    const std::int64_t rounded = (value + 0x800) >> 12;
    const std::int64_t result = rounded * gain;
    if (result < std::numeric_limits<std::int32_t>::min() ||
        result > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::int32_t>(result);
}

bool zero_band(const std::span<const std::int32_t> values) noexcept {
    for (const std::int32_t value : values) {
        if (value != 0) return false;
    }
    return true;
}

} // namespace

std::optional<std::uint16_t> module73_mpeg1_intensity_gain(
    const unsigned position) noexcept {
    if (position >= kMpeg1IntensityGains.size()) return std::nullopt;
    return kMpeg1IntensityGains[position];
}

std::optional<std::uint16_t> module73_lsf_intensity_gain(
    const unsigned intensity_scale, const unsigned position) noexcept {
    if (intensity_scale >= kLsfIntensityGains.size() ||
        position >= kLsfIntensityGains[0].size()) {
        return std::nullopt;
    }
    return kLsfIntensityGains[intensity_scale][position];
}

bool apply_module73_mpeg1_intensity_band(
    const std::span<std::int32_t> left,
    const std::span<std::int32_t> right,
    const unsigned position) noexcept {
    if (left.size() != right.size()) return false;
    if (position >= kMpeg1IntensityGains.size()) return true;
    const std::int64_t left_gain = kMpeg1IntensityGains[position];
    const std::int64_t right_gain =
        kMpeg1IntensityGains[kMpeg1IntensityGains.size() - 1U - position];
    for (std::size_t i = 0; i < left.size(); ++i) {
        const std::int64_t source = left[i];
        const auto reconstructed_left = rounded_q12_product(source, left_gain);
        const auto reconstructed_right = rounded_q12_product(source, right_gain);
        if (!reconstructed_left || !reconstructed_right) return false;
        left[i] = *reconstructed_left;
        right[i] = *reconstructed_right;
    }
    return true;
}

bool apply_module73_lsf_intensity_band(
    const std::span<std::int32_t> left,
    const std::span<std::int32_t> right,
    const unsigned position,
    const unsigned intensity_scale) noexcept {
    if (left.size() != right.size() ||
        intensity_scale >= kLsfIntensityGains.size() ||
        (position != 0 && (position - 1U) / 2U >= 15U)) {
        return false;
    }
    if (position == 0) {
        std::copy(left.begin(), left.end(), right.begin());
        return true;
    }
    const std::int64_t gain = kLsfIntensityGains[intensity_scale]
        [(position - 1U) / 2U];
    for (std::size_t i = 0; i < left.size(); ++i) {
        const std::int32_t source_left = left[i];
        const auto scaled = rounded_q12_product(source_left, gain);
        if (!scaled) return false;
        if ((position & 1U) != 0) {
            left[i] = *scaled;
            right[i] = source_left;
        } else {
            right[i] = *scaled;
        }
    }
    return true;
}

bool apply_module73_mid_side_stereo(
    const std::span<std::int32_t> left,
    const std::span<std::int32_t> right,
    const std::span<const std::uint8_t> transform_mask) noexcept {
    if (left.size() != right.size() || left.size() != transform_mask.size())
        return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (transform_mask[i] == 0) continue;
        const std::int64_t mid = static_cast<std::int64_t>(left[i]) + right[i];
        const std::int64_t side = static_cast<std::int64_t>(left[i]) - right[i];
        const auto reconstructed_left = rounded_q12_product(mid, kMidSideQ12);
        const auto reconstructed_right = rounded_q12_product(side, kMidSideQ12);
        if (!reconstructed_left || !reconstructed_right) return false;
        left[i] = *reconstructed_left;
        right[i] = *reconstructed_right;
    }
    return true;
}

bool apply_module73_mpeg1_layer3_stereo(
    const std::span<std::int32_t> left,
    const std::span<std::int32_t> right,
    const Module73Layer3ScaleFactors& left_scalefactors,
    const Module73Layer3ScaleFactors& right_scalefactors,
    const MpegLayer3GranuleChannel& left_granule,
    const MpegLayer3GranuleChannel& right_granule,
    const unsigned frequency_profile_slot,
    const bool intensity_stereo_enabled,
    const bool mid_side_stereo_enabled) noexcept {
    if (left.size() != 576 || right.size() != 576 ||
        left_granule.block_type != right_granule.block_type ||
        left_granule.mixed_block != right_granule.mixed_block) {
        return false;
    }
    std::array<std::uint8_t, 576> mid_side_mask{};
    if (mid_side_stereo_enabled) mid_side_mask.fill(1);

    if (intensity_stereo_enabled) {
        const auto shape = left_granule.block_type == 2
            ? (left_granule.mixed_block
                ? Module73Layer3BlockShape::mixed_block
                : Module73Layer3BlockShape::short_block)
            : Module73Layer3BlockShape::long_block;
        std::array<Module73Layer3SpectralBandPlan, 40> bands{};
        unsigned band_count = 0;
        unsigned mixed_long_band_count = 0;
        if (left_granule.block_type == 2) {
            const auto plan = plan_module73_layer3_short_spectral_bands(
                frequency_profile_slot, left_granule, shape, left_scalefactors);
            if (!plan) return false;
            band_count = plan->band_count;
            mixed_long_band_count = plan->mixed_long_band_count;
            std::copy_n(plan->bands.begin(), band_count, bands.begin());
        } else {
            const auto plan = plan_module73_layer3_long_spectral_bands(
                frequency_profile_slot, left_granule, left_scalefactors);
            if (!plan) return false;
            band_count = plan->band_count;
            std::copy_n(plan->bands.begin(), band_count, bands.begin());
        }

        const auto process_band = [&](const unsigned first_line,
                                      const unsigned line_count,
                                      const unsigned factor_index) {
            if (first_line + line_count > 576) return false;
            const auto right_band = std::span<const std::int32_t>(right)
                                        .subspan(first_line, line_count);
            if (!zero_band(right_band)) return true;
            const unsigned position = factor_index < right_scalefactors.values.size()
                ? right_scalefactors.values[factor_index] : 7U;
            if (position >= kMpeg1IntensityGains.size()) return true;
            if (!apply_module73_mpeg1_intensity_band(
                    left.subspan(first_line, line_count),
                    right.subspan(first_line, line_count), position)) {
                return false;
            }
            std::fill_n(mid_side_mask.begin() + first_line, line_count, 0);
            return true;
        };

        if (left_granule.block_type != 2) {
            for (unsigned band_index = 0; band_index < band_count;
                 ++band_index) {
                const auto& band = bands[band_index];
                if (!process_band(band.first_line, band.line_count,
                                  band.scalefactor_band)) {
                    return false;
                }
            }
        } else {
            for (unsigned band_index = 0; band_index < band_count;
                 ++band_index) {
                const auto& band = bands[band_index];
                if (band_index < mixed_long_band_count) {
                    if (!process_band(band.first_line, band.line_count,
                                      band.scalefactor_band)) {
                        return false;
                    }
                    continue;
                }
                if (band.line_count % 3U != 0) return false;
                const unsigned window_width = band.line_count / 3U;
                for (unsigned window = 0; window < 3; ++window) {
                    const unsigned first_line = band.first_line + window * window_width;
                    const unsigned factor_index = band.scalefactor_band + window;
                    if (!process_band(first_line, window_width, factor_index))
                        return false;
                }
            }
        }
    }
    return apply_module73_mid_side_stereo(left, right, mid_side_mask);
}

bool apply_module73_mpeg1_frame_stereo(
    Module73Layer3MainDataDecode& decoded_frame,
    const MpegLayer3SideInfo& side_info,
    const unsigned frequency_profile_slot,
    const bool intensity_stereo_enabled,
    const bool mid_side_stereo_enabled) noexcept {
    if (decoded_frame.granule_count != 2 || side_info.granule_count != 2 ||
        decoded_frame.channel_count != 2 || side_info.channel_count != 2) {
        return false;
    }
    auto candidate = decoded_frame;
    for (unsigned granule = 0; granule < 2; ++granule) {
        auto& left = candidate.granules[granule][0];
        auto& right = candidate.granules[granule][1];
        if (!apply_module73_mpeg1_layer3_stereo(
                left.spectral_values, right.spectral_values,
                left.scalefactors, right.scalefactors,
                side_info.granules[granule][0],
                side_info.granules[granule][1],
                frequency_profile_slot, intensity_stereo_enabled,
                mid_side_stereo_enabled)) {
            return false;
        }
    }
    decoded_frame = candidate;
    return true;
}

bool apply_module73_lsf_frame_stereo(
    Module73Layer3MainDataDecode& decoded_frame,
    const MpegLayer3SideInfo& side_info,
    const unsigned frequency_profile_slot,
    const bool intensity_stereo_enabled,
    const bool mid_side_stereo_enabled) noexcept {
    if (decoded_frame.granule_count != 1 || side_info.granule_count != 1 ||
        decoded_frame.channel_count != 2 || side_info.channel_count != 2) {
        return false;
    }
    auto candidate = decoded_frame;
    auto& left = candidate.granules[0][0];
    auto& right = candidate.granules[0][1];
    const auto& left_info = side_info.granules[0][0];
    const auto& right_info = side_info.granules[0][1];
    if (left_info.block_type != right_info.block_type ||
        left_info.mixed_block != right_info.mixed_block) {
        return false;
    }
    std::array<std::uint8_t, 576> mid_side_mask{};
    if (mid_side_stereo_enabled) mid_side_mask.fill(1);

    if (intensity_stereo_enabled) {
        const bool is_short = left_info.block_type == 2;
        const auto shape = is_short
            ? (left_info.mixed_block ? Module73Layer3BlockShape::mixed_block
                                     : Module73Layer3BlockShape::short_block)
            : Module73Layer3BlockShape::long_block;
        std::array<Module73Layer3SpectralBandPlan, 40> bands{};
        unsigned band_count = 0;
        unsigned mixed_long_band_count = 0;
        if (is_short) {
            const auto plan = plan_module73_layer3_short_spectral_bands(
                frequency_profile_slot, left_info, shape, left.scalefactors);
            if (!plan) return false;
            band_count = plan->band_count;
            mixed_long_band_count = plan->mixed_long_band_count;
            std::copy_n(plan->bands.begin(), band_count, bands.begin());
        } else {
            const auto plan = plan_module73_layer3_long_spectral_bands(
                frequency_profile_slot, left_info, left.scalefactors);
            if (!plan) return false;
            band_count = plan->band_count;
            std::copy_n(plan->bands.begin(), band_count, bands.begin());
        }
        const unsigned intensity_scale = right_info.scalefac_compress & 1U;
        const auto process_band = [&](const unsigned first_line,
                                      const unsigned line_count,
                                      const unsigned factor_index) {
            if (first_line + line_count > 576) return false;
            if (factor_index >= right.scalefactors.intensity_flags.size() ||
                right.scalefactors.intensity_flags[factor_index]) {
                return true;
            }
            const unsigned position = factor_index < right.scalefactors.value_count
                ? right.scalefactors.values[factor_index] : 0U;
            if (!apply_module73_lsf_intensity_band(
                    std::span<std::int32_t>(left.spectral_values)
                        .subspan(first_line, line_count),
                    std::span<std::int32_t>(right.spectral_values)
                        .subspan(first_line, line_count),
                    position, intensity_scale)) {
                return false;
            }
            std::fill_n(mid_side_mask.begin() + first_line, line_count, 0);
            return true;
        };
        for (unsigned band_index = 0; band_index < band_count; ++band_index) {
            const auto& band = bands[band_index];
            if (!is_short || band_index < mixed_long_band_count) {
                if (!process_band(band.first_line, band.line_count,
                                  band.scalefactor_band)) {
                    return false;
                }
            } else {
                if (band.line_count % 3U != 0) return false;
                const unsigned window_width = band.line_count / 3U;
                for (unsigned window = 0; window < 3; ++window) {
                    if (!process_band(
                            band.first_line + window * window_width,
                            window_width, band.scalefactor_band + window)) {
                        return false;
                    }
                }
            }
        }
    }
    if (!apply_module73_mid_side_stereo(left.spectral_values,
                                         right.spectral_values,
                                         mid_side_mask)) {
        return false;
    }
    decoded_frame = candidate;
    return true;
}

} // namespace echo::audio
