#pragma once

#include "echo/audio/module73_layer3_decode.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

// Q12 left/right gains recovered from module 73 data at 0x0102aee4.
[[nodiscard]] std::optional<std::uint16_t>
module73_mpeg1_intensity_gain(unsigned position) noexcept;

// Two LSF intensity coefficient rows recovered at 0x0102af00.
[[nodiscard]] std::optional<std::uint16_t>
module73_lsf_intensity_gain(unsigned intensity_scale,
                            unsigned position) noexcept;

// Reconstructs one MPEG-1 intensity-stereo band from the left-channel values.
// Position 7 or above denotes no intensity assignment and leaves buffers alone.
[[nodiscard]] bool apply_module73_mpeg1_intensity_band(
    std::span<std::int32_t> left,
    std::span<std::int32_t> right,
    unsigned position) noexcept;

[[nodiscard]] bool apply_module73_lsf_intensity_band(
    std::span<std::int32_t> left,
    std::span<std::int32_t> right,
    unsigned position,
    unsigned intensity_scale) noexcept;

// Applies module 73's fixed-point mid/side recurrence to samples selected by
// transform_mask. All spans must have the same length.
[[nodiscard]] bool apply_module73_mid_side_stereo(
    std::span<std::int32_t> left,
    std::span<std::int32_t> right,
    std::span<const std::uint8_t> transform_mask) noexcept;

// Applies MPEG-1 intensity stereo to empty right-channel bands, then optional
// mid/side processing to all remaining lines of one granule.
[[nodiscard]] bool apply_module73_mpeg1_layer3_stereo(
    std::span<std::int32_t> left,
    std::span<std::int32_t> right,
    const Module73Layer3ScaleFactors& left_scalefactors,
    const Module73Layer3ScaleFactors& right_scalefactors,
    const MpegLayer3GranuleChannel& left_granule,
    const MpegLayer3GranuleChannel& right_granule,
    unsigned frequency_profile_slot,
    bool intensity_stereo_enabled,
    bool mid_side_stereo_enabled) noexcept;

[[nodiscard]] bool apply_module73_mpeg1_frame_stereo(
    Module73Layer3MainDataDecode& decoded_frame,
    const MpegLayer3SideInfo& side_info,
    unsigned frequency_profile_slot,
    bool intensity_stereo_enabled,
    bool mid_side_stereo_enabled) noexcept;

[[nodiscard]] bool apply_module73_lsf_frame_stereo(
    Module73Layer3MainDataDecode& decoded_frame,
    const MpegLayer3SideInfo& side_info,
    unsigned frequency_profile_slot,
    bool intensity_stereo_enabled,
    bool mid_side_stereo_enabled) noexcept;

} // namespace echo::audio
