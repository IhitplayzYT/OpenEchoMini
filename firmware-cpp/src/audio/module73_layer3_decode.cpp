#include "echo/audio/module73_layer3_decode.hpp"

#include "echo/audio/module73_layer3_scalefactors.hpp"
#include "echo/audio/module73_layer3_stereo.hpp"

#include <algorithm>

namespace echo::audio {

namespace {

bool skip_main_data_bits(echo::bitstream::MsbBitReader& bits,
                         std::size_t count) noexcept {
    while (count != 0) {
        const unsigned step = static_cast<unsigned>(std::min<std::size_t>(count, 32));
        if (!bits.read(step)) return false;
        count -= step;
    }
    return true;
}

} // namespace

std::optional<Module73Layer3LongGranuleDecode>
decode_module73_layer3_long_granule(
    echo::bitstream::MsbBitReader& bits,
    const MpegLayer3GranuleChannel& granule,
    const bool mpeg1,
    const unsigned frequency_profile_slot,
    const bool intensity_stereo_right_channel,
    const std::array<bool, 4> scfsi,
    const std::span<const std::uint8_t> previous_granule_values) noexcept {
    if (granule.block_type == 2 ||
        (granule.window_switching && granule.block_type == 0) ||
        (!mpeg1 && intensity_stereo_right_channel)) {
        return std::nullopt;
    }

    auto cursor = bits;
    const std::size_t start = cursor.bits_consumed();
    Module73Layer3LongGranuleDecode result{};
    if (mpeg1) {
        const auto scalefactors = decode_module73_mpeg1_scalefactors(
            cursor, granule.scalefac_compress,
            Module73Layer3BlockShape::long_block, granule.preflag,
            scfsi, previous_granule_values);
        if (!scalefactors) return std::nullopt;
        result.scalefactors = *scalefactors;
    } else {
        const auto layout = module73_lsf_scalefactor_layout(
            granule.scalefac_compress,
            Module73Layer3BlockShape::long_block,
            intensity_stereo_right_channel);
        if (!layout) return std::nullopt;
        const auto scalefactors = decode_module73_lsf_scalefactors(cursor, *layout);
        if (!scalefactors) return std::nullopt;
        result.scalefactors = *scalefactors;
    }

    const std::size_t scalefactor_bits = cursor.bits_consumed() - start;
    if (scalefactor_bits > granule.part23_length) return std::nullopt;
    const auto plan = plan_module73_layer3_long_spectral_bands(
        frequency_profile_slot, granule, result.scalefactors);
    if (!plan) return std::nullopt;
    const unsigned huffman_budget = static_cast<unsigned>(
        granule.part23_length - scalefactor_bits);
    const auto spectral = decode_module73_layer3_long_spectral_data(
        cursor, *plan, granule.count1table_select, huffman_budget);
    if (!spectral) return std::nullopt;
    result.spectral = *spectral;

    const std::size_t total_bits = cursor.bits_consumed() - start;
    if (total_bits > granule.part23_length || total_bits > 0xffffffffU) {
        return std::nullopt;
    }
    result.part23_bits_consumed = static_cast<unsigned>(total_bits);
    bits = cursor;
    return result;
}

std::optional<Module73Layer3ShortGranuleDecode>
decode_module73_layer3_short_granule(
    echo::bitstream::MsbBitReader& bits,
    const MpegLayer3GranuleChannel& granule,
    const bool mpeg1,
    const unsigned frequency_profile_slot,
    const Module73Layer3BlockShape shape,
    const bool intensity_stereo_right_channel) noexcept {
    if (granule.block_type != 2 || shape == Module73Layer3BlockShape::long_block ||
        granule.mixed_block != (shape == Module73Layer3BlockShape::mixed_block) ||
        (!mpeg1 && intensity_stereo_right_channel)) {
        return std::nullopt;
    }

    auto cursor = bits;
    const std::size_t start = cursor.bits_consumed();
    Module73Layer3ShortGranuleDecode result{};
    if (mpeg1) {
        const auto scalefactors = decode_module73_mpeg1_scalefactors(
            cursor, granule.scalefac_compress, shape, granule.preflag);
        if (!scalefactors) return std::nullopt;
        result.scalefactors = *scalefactors;
    } else {
        const auto layout = module73_lsf_scalefactor_layout(
            granule.scalefac_compress, shape, intensity_stereo_right_channel);
        if (!layout) return std::nullopt;
        const auto scalefactors = decode_module73_lsf_scalefactors(cursor, *layout);
        if (!scalefactors) return std::nullopt;
        result.scalefactors = *scalefactors;
    }

    const std::size_t scalefactor_bits = cursor.bits_consumed() - start;
    if (scalefactor_bits > granule.part23_length) return std::nullopt;
    const auto plan = plan_module73_layer3_short_spectral_bands(
        frequency_profile_slot, granule, shape, result.scalefactors);
    if (!plan) return std::nullopt;
    const unsigned huffman_budget = static_cast<unsigned>(
        granule.part23_length - scalefactor_bits);
    const auto spectral = decode_module73_layer3_short_spectral_data(
        cursor, *plan, granule.count1table_select, huffman_budget);
    if (!spectral) return std::nullopt;
    result.spectral = *spectral;

    const std::size_t total_bits = cursor.bits_consumed() - start;
    if (total_bits > granule.part23_length || total_bits > 0xffffffffU)
        return std::nullopt;
    result.part23_bits_consumed = static_cast<unsigned>(total_bits);
    bits = cursor;
    return result;
}

std::optional<Module73Layer3MainDataDecode>
decode_module73_layer3_main_data(
    echo::bitstream::MsbBitReader& bits,
    const MpegLayer3SideInfo& side_info,
    const bool mpeg1,
    const unsigned frequency_profile_slot,
    const std::array<bool, 2> intensity_stereo_right_channel) noexcept {
    if (side_info.firmware_error != 0 || side_info.granule_count == 0 ||
        side_info.granule_count > 2 || side_info.channel_count == 0 ||
        side_info.channel_count > 2 ||
        mpeg1 != (side_info.granule_count == 2)) {
        return std::nullopt;
    }

    auto cursor = bits;
    Module73Layer3MainDataDecode result{};
    result.granule_count = side_info.granule_count;
    result.channel_count = side_info.channel_count;
    for (unsigned granule_index = 0;
         granule_index < side_info.granule_count; ++granule_index) {
        for (unsigned channel = 0; channel < side_info.channel_count; ++channel) {
            const auto& granule = side_info.granules[granule_index][channel];
            Module73Layer3MainDataGranule& output =
                result.granules[granule_index][channel];
            std::optional<unsigned> used_bits;
            if (granule.block_type == 2) {
                const auto shape = granule.mixed_block
                    ? Module73Layer3BlockShape::mixed_block
                    : Module73Layer3BlockShape::short_block;
                const auto decoded = decode_module73_layer3_short_granule(
                    cursor, granule, mpeg1, frequency_profile_slot, shape,
                    intensity_stereo_right_channel[channel]);
                if (!decoded) return std::nullopt;
                output.scalefactors = decoded->scalefactors;
                output.spectral_values = decoded->spectral.values;
                output.part23_bits_used = decoded->part23_bits_consumed;
                output.short_block = true;
                used_bits = decoded->part23_bits_consumed;
            } else {
                std::array<bool, 4> scfsi{};
                std::span<const std::uint8_t> previous_values{};
                if (mpeg1 && granule_index == 1) {
                    const unsigned mask = side_info.scfsi_masks[channel];
                    for (unsigned group = 0; group < scfsi.size(); ++group) {
                        scfsi[group] = (mask & (1U << (3U - group))) != 0;
                    }
                    previous_values = result.granules[0][channel]
                                          .scalefactors.values;
                }
                const auto decoded = decode_module73_layer3_long_granule(
                    cursor, granule, mpeg1, frequency_profile_slot,
                    intensity_stereo_right_channel[channel], scfsi,
                    previous_values);
                if (!decoded) return std::nullopt;
                output.scalefactors = decoded->scalefactors;
                output.spectral_values = decoded->spectral.values;
                output.part23_bits_used = decoded->part23_bits_consumed;
                used_bits = decoded->part23_bits_consumed;
            }

            if (*used_bits > granule.part23_length ||
                !skip_main_data_bits(cursor,
                    static_cast<std::size_t>(granule.part23_length - *used_bits))) {
                return std::nullopt;
            }
            if (result.part23_bits_total >
                0xffffffffU - granule.part23_length) return std::nullopt;
            result.part23_bits_total += granule.part23_length;
        }
    }
    bits = cursor;
    return result;
}

std::optional<Module73Layer3FrameDecode>
decode_module73_layer3_frame_main_data(
    const std::span<const std::uint8_t> previous_main_data,
    const std::span<const std::uint8_t> current_main_data,
    const MpegLayer3SideInfo& side_info,
    const bool mpeg1,
    const unsigned frequency_profile_slot,
    const std::span<std::uint8_t> main_data_window_scratch,
    const std::span<std::uint8_t> next_reservoir_storage,
    std::array<bool, 2> intensity_stereo_right_channel,
    const bool intensity_stereo_enabled,
    const bool mid_side_stereo_enabled) noexcept {
    const auto window = resolve_mpeg_layer3_main_data_window(
        previous_main_data, current_main_data, side_info.main_data_begin,
        main_data_window_scratch);
    if (!window) return std::nullopt;
    echo::bitstream::MsbBitReader reader(*window);
    if (!mpeg1 && side_info.channel_count == 2 && intensity_stereo_enabled)
        intensity_stereo_right_channel[1] = true;
    auto audio = decode_module73_layer3_main_data(
        reader, side_info, mpeg1, frequency_profile_slot,
        intensity_stereo_right_channel);
    if (!audio) return std::nullopt;
    if (side_info.channel_count == 2 &&
        (intensity_stereo_enabled || mid_side_stereo_enabled)) {
        const bool stereo_ok = mpeg1
            ? apply_module73_mpeg1_frame_stereo(
                *audio, side_info, frequency_profile_slot,
                intensity_stereo_enabled, mid_side_stereo_enabled)
            : apply_module73_lsf_frame_stereo(
                *audio, side_info, frequency_profile_slot,
                intensity_stereo_enabled, mid_side_stereo_enabled);
        if (!stereo_ok) return std::nullopt;
    }
    const auto next_reservoir = update_mpeg_layer3_reservoir_tail(
        previous_main_data, current_main_data, next_reservoir_storage);
    if (!next_reservoir) return std::nullopt;
    return Module73Layer3FrameDecode{
        *audio, *next_reservoir, window->size()};
}

} // namespace echo::audio
