#pragma once

#include "echo/audio/module73_layer3_spectral_plan.hpp"
#include "echo/audio/mpeg_reservoir.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct Module73Layer3LongGranuleDecode {
    Module73Layer3ScaleFactors scalefactors{};
    Module73Layer3LongSpectralDecode spectral{};
    unsigned part23_bits_consumed{};
};

struct Module73Layer3ShortGranuleDecode {
    Module73Layer3ScaleFactors scalefactors{};
    Module73Layer3ShortSpectralDecode spectral{};
    unsigned part23_bits_consumed{};
};

struct Module73Layer3MainDataGranule {
    Module73Layer3ScaleFactors scalefactors{};
    std::array<std::int32_t, 576> spectral_values{};
    unsigned part23_bits_used{};
    bool short_block{};
};

struct Module73Layer3MainDataDecode {
    std::array<std::array<Module73Layer3MainDataGranule, 2>, 2> granules{};
    std::uint8_t granule_count{};
    std::uint8_t channel_count{};
    unsigned part23_bits_total{};
};

struct Module73Layer3FrameDecode {
    Module73Layer3MainDataDecode audio{};
    std::span<const std::uint8_t> next_reservoir{};
    std::size_t main_data_window_bytes{};
};

// Reconstructs one non-short granule/channel from scalefactor bits through
// big_values and count1. The reader advances only on complete success. LSF
// intensity-stereo right-channel reconstruction and short/mixed blocks remain
// caller-owned.
[[nodiscard]] std::optional<Module73Layer3LongGranuleDecode>
decode_module73_layer3_long_granule(
    echo::bitstream::MsbBitReader& bits,
    const MpegLayer3GranuleChannel& granule,
    bool mpeg1,
    unsigned frequency_profile_slot,
    bool intensity_stereo_right_channel = false,
    std::array<bool, 4> scfsi = {},
    std::span<const std::uint8_t> previous_granule_values = {}) noexcept;

[[nodiscard]] std::optional<Module73Layer3ShortGranuleDecode>
decode_module73_layer3_short_granule(
    echo::bitstream::MsbBitReader& bits,
    const MpegLayer3GranuleChannel& granule,
    bool mpeg1,
    unsigned frequency_profile_slot,
    Module73Layer3BlockShape shape,
    bool intensity_stereo_right_channel = false) noexcept;

// Decodes granules in side-info order from a bit reader already positioned at
// the reservoir-resolved Layer III main-data start. The caller owns frame
// boundary parsing and reservoir-window assembly.
[[nodiscard]] std::optional<Module73Layer3MainDataDecode>
decode_module73_layer3_main_data(
    echo::bitstream::MsbBitReader& bits,
    const MpegLayer3SideInfo& side_info,
    bool mpeg1,
    unsigned frequency_profile_slot,
    std::array<bool, 2> intensity_stereo_right_channel = {}) noexcept;

// Resolves main_data_begin from caller-owned history, decodes this frame, and
// writes the next retained tail. The output spans must not overlap inputs.
[[nodiscard]] std::optional<Module73Layer3FrameDecode>
decode_module73_layer3_frame_main_data(
    std::span<const std::uint8_t> previous_main_data,
    std::span<const std::uint8_t> current_main_data,
    const MpegLayer3SideInfo& side_info,
    bool mpeg1,
    unsigned frequency_profile_slot,
    std::span<std::uint8_t> main_data_window_scratch,
    std::span<std::uint8_t> next_reservoir_storage,
    std::array<bool, 2> intensity_stereo_right_channel = {},
    bool intensity_stereo_enabled = false,
    bool mid_side_stereo_enabled = false) noexcept;

} // namespace echo::audio
