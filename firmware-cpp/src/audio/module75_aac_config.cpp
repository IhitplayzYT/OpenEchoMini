#include "echo/audio/module75_aac_config.hpp"

#include "echo/bitstream/msb_bit_reader.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace echo::audio {
namespace {

bool skip_bytes(std::span<const std::uint8_t> bytes,
                std::size_t& cursor,
                std::size_t count) noexcept {
    if (cursor > bytes.size() || count > bytes.size() - cursor) return false;
    cursor += count;
    return true;
}

bool read_byte(std::span<const std::uint8_t> bytes,
               std::size_t& cursor,
               std::uint8_t& value) noexcept {
    if (cursor >= bytes.size()) return false;
    value = bytes[cursor++];
    return true;
}

bool skip_descriptor_length(std::span<const std::uint8_t> bytes,
                            std::size_t& cursor,
                            std::uint32_t* value) noexcept {
    std::uint32_t decoded = 0;
    for (unsigned i = 0; i < 4; ++i) {
        std::uint8_t byte = 0;
        if (!read_byte(bytes, cursor, byte)) return false;
        decoded = (decoded << 7) | (byte & 0x7f);
        if ((byte & 0x80) == 0) {
            if (value != nullptr) *value = decoded;
            return true;
        }
    }
    if (value != nullptr) *value = decoded;
    return true;
}

}  // namespace

Module75AacConfigStatus parse_module75_aac_config_descriptors(
    std::span<const std::uint8_t> bytes,
    Module75AacConfigIndices& output) noexcept {
    std::size_t cursor = 0;
    if (!skip_bytes(bytes, cursor, 4)) return Module75AacConfigStatus::input_error;

    std::uint8_t tag = 0;
    if (!read_byte(bytes, cursor, tag)) return Module75AacConfigStatus::input_error;
    if (tag == 0x03) {
        if (!skip_descriptor_length(bytes, cursor, nullptr) ||
            !skip_bytes(bytes, cursor, 3))
            return Module75AacConfigStatus::input_error;
    } else if (!skip_bytes(bytes, cursor, 2)) {
        return Module75AacConfigStatus::input_error;
    }

    if (!read_byte(bytes, cursor, tag)) return Module75AacConfigStatus::input_error;
    if (tag != 0x04) return Module75AacConfigStatus::invalid_descriptor;

    std::uint32_t decoder_config_length = 0;
    if (!skip_descriptor_length(bytes, cursor, &decoder_config_length) ||
        decoder_config_length < 0x0d)
        return Module75AacConfigStatus::invalid_descriptor;
    if (!skip_bytes(bytes, cursor, 0x0d)) return Module75AacConfigStatus::input_error;

    if (!read_byte(bytes, cursor, tag)) return Module75AacConfigStatus::input_error;
    if (tag != 0x05) return Module75AacConfigStatus::invalid_descriptor;
    if (!skip_descriptor_length(bytes, cursor, nullptr))
        return Module75AacConfigStatus::input_error;

    Module75AacConfigIndices parsed{};
    if (!read_byte(bytes, cursor, parsed.source_config_byte_0) ||
        !read_byte(bytes, cursor, parsed.source_config_byte_1))
        return Module75AacConfigStatus::input_error;

    parsed.frequency_index = static_cast<std::uint8_t>(
        ((parsed.source_config_byte_0 & 0x07) << 1) |
        (parsed.source_config_byte_1 >> 7));
    parsed.channel_configuration_index = static_cast<std::uint8_t>(
        (parsed.source_config_byte_1 & 0x7f) >> 3);
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_program_config(
    std::span<const std::uint8_t> bytes,
    Module75AacProgramConfig& output) noexcept {
    echo::bitstream::MsbBitReader reader(bytes);
    Module75AacProgramConfig parsed{};
    auto read_field = [&reader](unsigned width, std::uint8_t& destination) noexcept {
        const auto value = reader.read(width);
        if (!value) return false;
        destination = static_cast<std::uint8_t>(*value);
        return true;
    };
    std::uint8_t bit = 0;

    if (!read_field(4, parsed.element_instance_tag) ||
        !read_field(2, parsed.object_type) ||
        !read_field(4, parsed.sampling_frequency_index) ||
        !read_field(4, parsed.front_count) ||
        !read_field(4, parsed.side_count) ||
        !read_field(4, parsed.back_count) ||
        !read_field(2, parsed.lfe_count) ||
        !read_field(3, parsed.associated_data_count) ||
        !read_field(4, parsed.coupling_channel_count))
        return Module75AacConfigStatus::input_error;

    if (!read_field(1, bit)) return Module75AacConfigStatus::input_error;
    parsed.mono_mixdown_present = bit != 0;
    if (parsed.mono_mixdown_present &&
        !read_field(4, parsed.mono_mixdown_element_number))
        return Module75AacConfigStatus::input_error;

    if (!read_field(1, bit)) return Module75AacConfigStatus::input_error;
    parsed.stereo_mixdown_present = bit != 0;
    if (parsed.stereo_mixdown_present &&
        !read_field(4, parsed.stereo_mixdown_element_number))
        return Module75AacConfigStatus::input_error;

    if (!read_field(1, bit)) return Module75AacConfigStatus::input_error;
    parsed.matrix_mixdown_present = bit != 0;
    if (parsed.matrix_mixdown_present &&
        (!read_field(2, parsed.matrix_mixdown_index) ||
         !read_field(1, bit)))
        return Module75AacConfigStatus::input_error;
    parsed.pseudo_surround_enable = parsed.matrix_mixdown_present && bit != 0;

    auto read_elements = [&read_field](auto& elements, std::uint8_t count) noexcept {
        for (std::size_t i = 0; i < count; ++i) {
            std::uint8_t is_pair = 0;
            if (!read_field(1, is_pair) || !read_field(4, elements[i].tag_select))
                return false;
            elements[i].is_channel_pair = is_pair != 0;
        }
        return true;
    };
    if (!read_elements(parsed.front, parsed.front_count) ||
        !read_elements(parsed.side, parsed.side_count) ||
        !read_elements(parsed.back, parsed.back_count))
        return Module75AacConfigStatus::input_error;

    for (std::size_t i = 0; i < parsed.lfe_count; ++i)
        if (!read_field(4, parsed.lfe_tag_select[i]))
            return Module75AacConfigStatus::input_error;
    for (std::size_t i = 0; i < parsed.associated_data_count; ++i)
        if (!read_field(4, parsed.associated_data_tag_select[i]))
            return Module75AacConfigStatus::input_error;
    for (std::size_t i = 0; i < parsed.coupling_channel_count; ++i) {
        if (!read_field(1, bit) ||
            !read_field(4, parsed.coupling_channel[i].tag_select))
            return Module75AacConfigStatus::input_error;
        parsed.coupling_channel[i].independent_switch = bit != 0;
    }

    const auto alignment = static_cast<unsigned>((8U - reader.bits_consumed() % 8U) % 8U);
    if (alignment != 0 && !reader.read(alignment))
        return Module75AacConfigStatus::input_error;
    if (!read_field(8, parsed.comment_length))
        return Module75AacConfigStatus::input_error;
    parsed.comment_offset = reader.bits_consumed() / 8U;
    for (std::uint16_t i = 0; i < parsed.comment_length; ++i) {
        if (!reader.read(8)) return Module75AacConfigStatus::input_error;
    }
    parsed.bytes_consumed = reader.bits_consumed() / 8U;
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_ics_info(
    std::span<const std::uint8_t> bits,
    std::uint8_t max_prediction_sfb,
    Module75AacIcsInfo& output,
    const std::size_t start_bit_offset) noexcept {
    echo::bitstream::MsbBitReader reader(bits, start_bit_offset);
    Module75AacIcsInfo parsed{};
    auto read_field = [&reader](unsigned width, std::uint8_t& destination) noexcept {
        const auto value = reader.read(width);
        if (!value) return false;
        destination = static_cast<std::uint8_t>(*value);
        return true;
    };
    std::uint8_t value = 0;

    if (!read_field(1, value)) return Module75AacConfigStatus::input_error;
    parsed.reserved_bit = value != 0;
    if (!read_field(2, parsed.window_sequence) ||
        !read_field(1, value))
        return Module75AacConfigStatus::input_error;
    parsed.window_shape = value != 0;

    if (parsed.window_sequence == 2) {
        if (!read_field(4, parsed.max_sfb) || !read_field(7, value))
            return Module75AacConfigStatus::input_error;
        parsed.window_group_count = 1;
        parsed.window_group_length[0] = 1;
        for (unsigned i = 0; i < 7; ++i) {
            if ((value & (0x40U >> i)) == 0) {
                ++parsed.window_group_count;
                parsed.window_group_length[parsed.window_group_count - 1] = 1;
            } else {
                ++parsed.window_group_length[parsed.window_group_count - 1];
            }
        }
    } else {
        if (!read_field(6, parsed.max_sfb) || !read_field(1, value))
            return Module75AacConfigStatus::input_error;
        parsed.predictor_data_present = value != 0;
        parsed.window_group_count = 1;
        parsed.window_group_length[0] = 1;
        if (parsed.predictor_data_present) {
            if (!read_field(1, value)) return Module75AacConfigStatus::input_error;
            parsed.predictor_reset = value != 0;
            if (parsed.predictor_reset &&
                !read_field(5, parsed.predictor_reset_group))
                return Module75AacConfigStatus::input_error;
            parsed.predictor_flag_count =
                parsed.max_sfb < max_prediction_sfb ? parsed.max_sfb : max_prediction_sfb;
            for (std::size_t i = 0; i < parsed.predictor_flag_count; ++i) {
                if (!read_field(1, value)) return Module75AacConfigStatus::input_error;
                parsed.predictor_used[i] = value != 0;
            }
        }
    }
    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_channel_pair_header(
    std::span<const std::uint8_t> bits,
    const std::uint8_t max_prediction_sfb,
    Module75AacChannelPairHeader& output,
    const std::size_t start_bit_offset) noexcept {
    echo::bitstream::MsbBitReader reader(bits, start_bit_offset);
    Module75AacChannelPairHeader parsed{};
    const auto tag = reader.read(4);
    const auto common_window = reader.read(1);
    if (!tag || !common_window)
        return Module75AacConfigStatus::input_error;
    parsed.element_instance_tag = static_cast<std::uint8_t>(*tag);
    parsed.common_window = *common_window != 0;
    if (!parsed.common_window) {
        parsed.bits_consumed = reader.bits_consumed();
        output = parsed;
        return Module75AacConfigStatus::ok;
    }

    const auto ics_start = reader.absolute_bit_position();
    const auto ics_status = parse_module75_aac_ics_info(
        bits, max_prediction_sfb, parsed.ics_info, ics_start);
    if (ics_status != Module75AacConfigStatus::ok) return ics_status;
    if (parsed.ics_info.bits_consumed > reader.bits_remaining())
        return Module75AacConfigStatus::input_error;
    const auto mask_start = ics_start + parsed.ics_info.bits_consumed;
    echo::bitstream::MsbBitReader mask_reader(bits, mask_start);
    const auto mask_present = mask_reader.read(2);
    if (!mask_present) return Module75AacConfigStatus::input_error;
    parsed.ms_mask_present = static_cast<std::uint8_t>(*mask_present);
    if (parsed.ms_mask_present == 1) {
        const auto mask_count = static_cast<std::size_t>(
            parsed.ics_info.window_group_count) * parsed.ics_info.max_sfb;
        if (mask_count > parsed.ms_used_by_group_sfb.size())
            return Module75AacConfigStatus::invalid_descriptor;
        for (std::size_t i = 0; i < mask_count; ++i) {
            const auto used = mask_reader.read(1);
            if (!used) return Module75AacConfigStatus::input_error;
            parsed.ms_used_by_group_sfb[i] = *used != 0;
        }
        parsed.ms_mask_bit_count = mask_count;
    }
    parsed.bits_consumed = (mask_start - start_bit_offset) +
                           mask_reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_single_element_header(
    std::span<const std::uint8_t> bits,
    const bool low_frequency_effect,
    Module75AacSingleElementHeader& output,
    const std::size_t start_bit_offset) noexcept {
    echo::bitstream::MsbBitReader reader(bits, start_bit_offset);
    const auto tag = reader.read(4);
    if (!tag) return Module75AacConfigStatus::input_error;
    output = {static_cast<std::uint8_t>(*tag), low_frequency_effect,
              reader.bits_consumed()};
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_section_data(
    std::span<const std::uint8_t> bits,
    const Module75AacIcsInfo& ics,
    Module75AacSectionData& output) noexcept {
    if (ics.window_group_count == 0 || ics.window_group_count > 8 ||
        ics.max_sfb > 64)
        return Module75AacConfigStatus::invalid_descriptor;

    echo::bitstream::MsbBitReader reader(bits);
    Module75AacSectionData parsed{};
    parsed.window_group_count = ics.window_group_count;
    parsed.max_sfb = ics.max_sfb;
    parsed.section_length_bits = ics.window_sequence == 2 ? 3 : 5;
    const auto escape_length = static_cast<std::uint32_t>(
        (1U << parsed.section_length_bits) - 1U);

    for (std::size_t group = 0; group < parsed.window_group_count; ++group) {
        std::size_t first_sfb = 0;
        while (first_sfb < parsed.max_sfb) {
            if (parsed.section_count[group] >= 64)
                return Module75AacConfigStatus::invalid_descriptor;
            const auto codebook = reader.read(4);
            if (!codebook) return Module75AacConfigStatus::input_error;

            std::uint32_t run_length = 0;
            std::uint32_t part = 0;
            do {
                const auto value = reader.read(parsed.section_length_bits);
                if (!value) return Module75AacConfigStatus::input_error;
                part = *value;
                run_length += part;
            } while (part == escape_length);

            const auto remaining = static_cast<std::size_t>(parsed.max_sfb) - first_sfb;
            if (run_length == 0 || run_length > remaining)
                return Module75AacConfigStatus::invalid_descriptor;

            const auto section_index = parsed.section_count[group]++;
            parsed.sections[group][section_index] = {
                static_cast<std::uint8_t>(*codebook),
                static_cast<std::uint8_t>(first_sfb),
                static_cast<std::uint8_t>(run_length)};
            for (std::size_t sfb = first_sfb; sfb < first_sfb + run_length; ++sfb)
                parsed.codebook_by_group_sfb[group * 64 + sfb] =
                    static_cast<std::uint8_t>(*codebook);
            first_sfb += run_length;
        }
    }
    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_scalefactors(
    std::span<const std::uint8_t> bits,
    const Module75AacSectionData& sections,
    const std::span<const Module75AacHuffmanTableView, 16> tables_by_codebook,
    Module75AacScalefactors& output) noexcept {
    if (sections.window_group_count == 0 || sections.window_group_count > 8 ||
        sections.max_sfb > 64)
        return Module75AacConfigStatus::invalid_descriptor;

    echo::bitstream::MsbBitReader reader(bits);
    Module75AacScalefactors parsed{};
    const auto global_gain = reader.read(8);
    if (!global_gain) return Module75AacConfigStatus::input_error;
    parsed.global_gain = static_cast<std::uint8_t>(*global_gain);
    std::int32_t scale_factor = static_cast<std::int32_t>(*global_gain);
    std::int32_t intensity_position = 0;
    std::int32_t noise_energy = static_cast<std::int32_t>(*global_gain) - 0x15a;
    bool first_noise_band = true;

    auto decode_delta = [&reader](const Module75AacHuffmanTableView& table,
                                  std::int16_t& symbol) noexcept {
        const auto max_depth = table.count_by_depth.size() < 32
                                   ? table.count_by_depth.size()
                                   : 32;
        if (max_depth == 0) return false;
        std::uint32_t prefix = 0;
        for (std::size_t depth = 0; depth < max_depth; ++depth) {
            const auto bit = reader.read(1);
            if (!bit) return false;
            prefix = static_cast<std::uint32_t>((prefix << 1U) | *bit);
            const auto codeword = depth == 31
                                      ? prefix
                                      : static_cast<std::uint32_t>(prefix << (31U - depth));
            const auto decoded = decode_module75_aac_huffman_symbol(
                codeword, table.count_by_depth, table.symbol_table,
                table.symbol_table_base);
            if (decoded) {
                if (decoded->bits_consumed != depth + 1U) return false;
                symbol = decoded->value;
                return true;
            }
        }
        return false;
    };

    for (std::size_t group = 0; group < sections.window_group_count; ++group) {
        for (std::size_t sfb = 0; sfb < sections.max_sfb; ++sfb) {
            const auto codebook = sections.codebook_by_group_sfb[group * 64U + sfb];
            std::int32_t value = 0;
            if (codebook >= 1 && codebook <= 11) {
                std::int16_t delta = 0;
                if (!decode_delta(tables_by_codebook[codebook], delta))
                    return Module75AacConfigStatus::input_error;
                scale_factor += delta;
                value = scale_factor;
            } else if (codebook == 13) {
                if (first_noise_band) {
                    const auto delta = reader.read(9);
                    if (!delta) return Module75AacConfigStatus::input_error;
                    noise_energy += static_cast<std::int32_t>(*delta);
                    first_noise_band = false;
                } else {
                    std::int16_t delta = 0;
                    if (!decode_delta(tables_by_codebook[codebook], delta))
                        return Module75AacConfigStatus::input_error;
                    noise_energy += delta;
                }
                value = noise_energy;
            } else if (codebook == 14 || codebook == 15) {
                std::int16_t delta = 0;
                if (!decode_delta(tables_by_codebook[codebook], delta))
                    return Module75AacConfigStatus::input_error;
                intensity_position += delta;
                value = intensity_position;
            }
            parsed.value_by_group_sfb[group * 64U + sfb] = value;
            ++parsed.scalefactor_count;
        }
    }
    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_pulse_data(
    std::span<const std::uint8_t> bits,
    Module75AacPulseData& output) noexcept {
    echo::bitstream::MsbBitReader reader(bits);
    Module75AacPulseData parsed{};
    const auto present = reader.read(1);
    if (!present) return Module75AacConfigStatus::input_error;
    parsed.present = *present != 0;
    if (parsed.present) {
        const auto count_minus_one = reader.read(2);
        const auto start_sfb = reader.read(6);
        if (!count_minus_one || !start_sfb)
            return Module75AacConfigStatus::input_error;
        parsed.pulse_count = static_cast<std::uint8_t>(*count_minus_one + 1U);
        parsed.pulse_start_sfb = static_cast<std::uint8_t>(*start_sfb);
        for (std::size_t i = 0; i < parsed.pulse_count; ++i) {
            const auto offset = reader.read(5);
            const auto amplitude = reader.read(4);
            if (!offset || !amplitude)
                return Module75AacConfigStatus::input_error;
            parsed.offset[i] = static_cast<std::uint8_t>(*offset);
            parsed.amplitude[i] = static_cast<std::uint8_t>(*amplitude);
        }
    }
    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus apply_module75_aac_pulse_corrections(
    const std::span<std::int32_t> spectral_lines,
    const std::span<const std::uint16_t> scalefactor_band_offsets,
    const Module75AacPulseData& pulse) noexcept {
    if (!pulse.present) return Module75AacConfigStatus::ok;
    if (pulse.pulse_count == 0 || pulse.pulse_count > pulse.offset.size() ||
        pulse.pulse_start_sfb >= scalefactor_band_offsets.size())
        return Module75AacConfigStatus::invalid_descriptor;

    std::array<std::size_t, 4> positions{};
    std::size_t position = scalefactor_band_offsets[pulse.pulse_start_sfb];
    for (std::size_t i = 0; i < pulse.pulse_count; ++i) {
        if (pulse.offset[i] > std::numeric_limits<std::size_t>::max() - position)
            return Module75AacConfigStatus::invalid_descriptor;
        position += pulse.offset[i];
        if (position >= spectral_lines.size())
            return Module75AacConfigStatus::input_error;
        positions[i] = position;
    }

    for (std::size_t i = 0; i < pulse.pulse_count; ++i) {
        const auto amplitude = static_cast<std::int32_t>(pulse.amplitude[i]);
        const auto delta = spectral_lines[positions[i]] < 1 ? -amplitude : amplitude;
        if ((delta > 0 && spectral_lines[positions[i]] >
                              std::numeric_limits<std::int32_t>::max() - delta) ||
            (delta < 0 && spectral_lines[positions[i]] <
                              std::numeric_limits<std::int32_t>::min() - delta))
            return Module75AacConfigStatus::invalid_descriptor;
        spectral_lines[positions[i]] += delta;
    }
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_tns_data(
    std::span<const std::uint8_t> bits,
    const bool short_window,
    Module75AacTnsData& output) noexcept {
    echo::bitstream::MsbBitReader reader(bits);
    Module75AacTnsData parsed{};
    parsed.short_window = short_window;
    parsed.window_count = short_window ? 8 : 1;
    const auto present = reader.read(1);
    if (!present) return Module75AacConfigStatus::input_error;
    parsed.present = *present != 0;

    if (parsed.present) {
        const unsigned filter_count_bits = short_window ? 1U : 2U;
        const unsigned length_bits = short_window ? 4U : 6U;
        const unsigned order_bits = short_window ? 3U : 5U;
        for (std::size_t window = 0; window < parsed.window_count; ++window) {
            const auto filter_count = reader.read(filter_count_bits);
            if (!filter_count) return Module75AacConfigStatus::input_error;
            parsed.filter_count[window] = static_cast<std::uint8_t>(*filter_count);
            if (*filter_count == 0) continue;

            const auto coefficient_resolution = reader.read(1);
            if (!coefficient_resolution) return Module75AacConfigStatus::input_error;
            for (std::size_t filter_index = 0; filter_index < *filter_count;
                 ++filter_index) {
                auto& filter = parsed.filters[window][filter_index];
                const auto length = reader.read(length_bits);
                const auto order = reader.read(order_bits);
                if (!length || !order) return Module75AacConfigStatus::input_error;
                filter.length = static_cast<std::uint8_t>(*length);
                filter.order = static_cast<std::uint8_t>(*order);
                if (filter.order == 0) continue;

                const auto direction = reader.read(1);
                const auto compressed = reader.read(1);
                if (!direction || !compressed)
                    return Module75AacConfigStatus::input_error;
                filter.direction = *direction != 0;
                filter.coefficient_compressed = *compressed != 0;
                const auto coefficient_bits = static_cast<unsigned>(
                    3U + *coefficient_resolution - *compressed);
                filter.coefficient_count = filter.order;
                for (std::size_t i = 0; i < filter.coefficient_count; ++i) {
                    const auto coefficient = reader.read(coefficient_bits);
                    if (!coefficient) return Module75AacConfigStatus::input_error;
                    filter.coefficients[i] = static_cast<std::uint8_t>(*coefficient);
                }
            }
        }
    }
    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_gain_control_data(
    std::span<const std::uint8_t> bits,
    const Module75AacGainControlLayout& layout,
    Module75AacGainControlData& output) noexcept {
    if (layout.windows_per_band == 0 || layout.windows_per_band > 8 ||
        layout.first_window_location_bits == 0 ||
        layout.first_window_location_bits > 8 ||
        layout.later_window_location_bits == 0 ||
        layout.later_window_location_bits > 8)
        return Module75AacConfigStatus::invalid_descriptor;

    echo::bitstream::MsbBitReader reader(bits);
    Module75AacGainControlData parsed{};
    const auto present = reader.read(1);
    if (!present) return Module75AacConfigStatus::input_error;
    parsed.present = *present != 0;
    if (parsed.present) {
        const auto max_band = reader.read(2);
        if (!max_band) return Module75AacConfigStatus::input_error;
        parsed.max_band = static_cast<std::uint8_t>(*max_band);
        for (std::size_t band = 1; band <= parsed.max_band; ++band) {
            for (std::size_t window = 0; window < layout.windows_per_band; ++window) {
                const auto count = reader.read(3);
                if (!count) return Module75AacConfigStatus::input_error;
                parsed.adjustment_count[band][window] =
                    static_cast<std::uint8_t>(*count);
                const auto location_bits = window == 0
                                               ? layout.first_window_location_bits
                                               : layout.later_window_location_bits;
                for (std::size_t i = 0; i < *count; ++i) {
                    const auto level = reader.read(4);
                    const auto location = reader.read(location_bits);
                    if (!level || !location)
                        return Module75AacConfigStatus::input_error;
                    parsed.adjustments[band][window][i] = {
                        static_cast<std::uint8_t>(*level),
                        static_cast<std::uint8_t>(*location)};
                }
            }
        }
    }
    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_data_stream_element(
    std::span<const std::uint8_t> bits,
    const std::size_t start_bit_after_element_id,
    Module75AacPayloadSpan& output) noexcept {
    echo::bitstream::MsbBitReader reader(bits, start_bit_after_element_id);
    Module75AacPayloadSpan parsed{};
    const auto tag = reader.read(4);
    const auto align = reader.read(1);
    const auto initial_length = reader.read(8);
    if (!tag || !align || !initial_length)
        return Module75AacConfigStatus::input_error;

    std::uint32_t length = *initial_length;
    if (length == 0xffU) {
        const auto extension = reader.read(8);
        if (!extension) return Module75AacConfigStatus::input_error;
        length += *extension;
    }

    parsed.element_instance_tag = static_cast<std::uint8_t>(*tag);
    parsed.byte_align = *align != 0;
    if (parsed.byte_align) {
        const auto padding = static_cast<unsigned>(
            (8U - reader.absolute_bit_position() % 8U) % 8U);
        if (padding != 0 && !reader.read(padding))
            return Module75AacConfigStatus::input_error;
    }

    parsed.payload_bit_offset = reader.absolute_bit_position();
    parsed.payload_length = static_cast<std::uint16_t>(length);
    if (length > reader.bits_remaining() / 8U)
        return Module75AacConfigStatus::input_error;
    for (std::uint32_t i = 0; i < length; ++i) {
        if (!reader.read(8)) return Module75AacConfigStatus::input_error;
    }
    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_fill_element(
    std::span<const std::uint8_t> bits,
    const std::size_t start_bit_after_element_id,
    Module75AacPayloadSpan& output) noexcept {
    echo::bitstream::MsbBitReader reader(bits, start_bit_after_element_id);
    Module75AacPayloadSpan parsed{};
    const auto initial_length = reader.read(4);
    if (!initial_length) return Module75AacConfigStatus::input_error;

    std::uint32_t length = *initial_length;
    if (length == 15U) {
        const auto escape = reader.read(8);
        if (!escape) return Module75AacConfigStatus::input_error;
        length += *escape + 14U;
    }

    parsed.payload_bit_offset = reader.absolute_bit_position();
    parsed.payload_length = static_cast<std::uint16_t>(length);
    if (length > reader.bits_remaining() / 8U)
        return Module75AacConfigStatus::input_error;
    for (std::uint32_t i = 0; i < length; ++i) {
        if (!reader.read(8)) return Module75AacConfigStatus::input_error;
    }
    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

std::optional<Module75AacHuffmanSymbol> decode_module75_aac_huffman_symbol(
    std::uint32_t codeword,
    std::span<const std::uint8_t> count_by_depth,
    std::span<const std::int16_t> symbol_table,
    std::size_t symbol_table_base) noexcept {
    std::uint64_t node_base = 0;
    std::size_t row_base = 0;
    const auto max_depth = count_by_depth.size() < 32 ? count_by_depth.size() : 32;
    for (std::size_t depth = 0; depth < max_depth; ++depth) {
        const auto bits = static_cast<unsigned>(depth + 1);
        const auto prefix = bits == 32 ? codeword : codeword >> (32U - bits);
        const auto relative_base = node_base * 2U;
        const auto count = static_cast<std::uint64_t>(count_by_depth[depth]);
        if (prefix >= relative_base) {
            const auto index = static_cast<std::uint64_t>(prefix) - relative_base;
            if (index < count) {
                const auto table_index = symbol_table_base + row_base +
                                         static_cast<std::size_t>(index);
                if (table_index >= symbol_table.size()) return std::nullopt;
                return Module75AacHuffmanSymbol{
                    symbol_table[table_index], static_cast<std::uint8_t>(bits)};
            }
        }
        row_base += static_cast<std::size_t>(count);
        node_base = relative_base + count;
    }
    return std::nullopt;
}

Module75AacConfigStatus parse_module75_aac_spectral_vector(
    std::span<const std::uint8_t> bits,
    const std::uint8_t codebook,
    const Module75AacHuffmanTableView& table,
    Module75AacSpectralVector& output,
    const std::size_t start_bit_offset) noexcept {
    if (codebook == 0 || codebook > 11 || table.count_by_depth.empty())
        return Module75AacConfigStatus::invalid_descriptor;

    echo::bitstream::MsbBitReader reader(bits, start_bit_offset);
    std::uint32_t prefix = 0;
    std::optional<Module75AacHuffmanSymbol> decoded;
    const auto max_depth = table.count_by_depth.size() < 32
                               ? table.count_by_depth.size()
                               : 32;
    for (std::size_t depth = 0; depth < max_depth; ++depth) {
        const auto bit = reader.read(1);
        if (!bit) return Module75AacConfigStatus::input_error;
        prefix = static_cast<std::uint32_t>((prefix << 1U) | *bit);
        const auto codeword = depth == 31
                                  ? prefix
                                  : static_cast<std::uint32_t>(prefix << (31U - depth));
        decoded = decode_module75_aac_huffman_symbol(
            codeword, table.count_by_depth, table.symbol_table,
            table.symbol_table_base);
        if (decoded) break;
    }
    if (!decoded || decoded->value < 0)
        return Module75AacConfigStatus::invalid_descriptor;

    const auto width = codebook <= 4 ? 3U : (codebook <= 10 ? 5U : 6U);
    const auto dimension = codebook <= 4 ? 4U : 2U;
    const auto metadata_width = codebook <= 4 ? 3U : 2U;
    const auto packed_mask = codebook <= 4 ? 0x7fffU
                             : codebook <= 10 ? 0x0fffU
                                              : 0x3fffU;
    const auto packed = static_cast<std::uint32_t>(decoded->value) & packed_mask;
    const auto sign_count = static_cast<std::uint8_t>(packed >> (dimension * width));
    Module75AacSpectralVector parsed{};
    parsed.codebook = codebook;
    parsed.dimension = static_cast<std::uint8_t>(dimension);
    std::array<bool, 4> sign_present{};
    std::uint8_t nonzero_count = 0;
    for (std::size_t i = 0; i < dimension; ++i) {
        const auto shift = static_cast<unsigned>((dimension - i - 1U) * width);
        const auto raw = (packed >> shift) & ((1U << width) - 1U);
        const auto sign_bit = 1U << (width - 1U);
        parsed.coefficients[i] = (raw & sign_bit) != 0
                                     ? static_cast<std::int32_t>(raw) -
                                           static_cast<std::int32_t>(1U << width)
                                     : static_cast<std::int32_t>(raw);
        sign_present[i] = parsed.coefficients[i] != 0;
        if (sign_present[i]) ++nonzero_count;
    }
    if (sign_count != nonzero_count || sign_count > dimension ||
        metadata_width + dimension * width > (codebook <= 4 ? 15U
                                                   : codebook <= 10 ? 12U
                                                                    : 14U))
        return Module75AacConfigStatus::invalid_descriptor;

    parsed.sign_bits_consumed = sign_count;
    for (std::size_t i = 0; i < dimension; ++i) {
        if (!sign_present[i]) continue;
        const auto sign = reader.read(1);
        if (!sign) return Module75AacConfigStatus::input_error;
        if (*sign != 0) parsed.coefficients[i] = -parsed.coefficients[i];
    }

    if (codebook == 11) {
        for (std::size_t i = 0; i < dimension; ++i) {
            if (parsed.coefficients[i] != 16 && parsed.coefficients[i] != -16)
                continue;
            unsigned escape_width = 4;
            for (;;) {
                const auto more = reader.read(1);
                if (!more) return Module75AacConfigStatus::input_error;
                if (*more == 0) break;
                if (escape_width == 30)
                    return Module75AacConfigStatus::invalid_descriptor;
                ++escape_width;
            }
            const auto extra = reader.read(escape_width);
            if (!extra) return Module75AacConfigStatus::input_error;
            const auto magnitude = static_cast<std::int32_t>(1U << escape_width) +
                                   static_cast<std::int32_t>(*extra);
            parsed.coefficients[i] = parsed.coefficients[i] < 0 ? -magnitude : magnitude;
        }
    }

    parsed.bits_consumed = reader.bits_consumed();
    output = parsed;
    return Module75AacConfigStatus::ok;
}

Module75AacConfigStatus parse_module75_aac_spectral_band(
    std::span<const std::uint8_t> bits,
    const std::uint8_t codebook,
    const std::uint16_t line_count,
    const Module75AacHuffmanTableView& table,
    Module75AacSpectralBand& output) noexcept {
    if (line_count > 1024)
        return Module75AacConfigStatus::invalid_descriptor;

    Module75AacSpectralBand parsed{};
    parsed.codebook = codebook;
    parsed.line_count = line_count;
    if (codebook == 0 || codebook > 11) {
        output = parsed;
        return Module75AacConfigStatus::ok;
    }
    const auto vector_width = static_cast<std::uint16_t>(codebook <= 4 ? 4 : 2);
    if (line_count % vector_width != 0)
        return Module75AacConfigStatus::invalid_descriptor;

    std::size_t bit_offset = 0;
    while (parsed.vector_count < line_count / vector_width) {
        Module75AacSpectralVector vector{};
        const auto status = parse_module75_aac_spectral_vector(
            bits, codebook, table, vector, bit_offset);
        if (status != Module75AacConfigStatus::ok) return status;
        const auto destination = static_cast<std::size_t>(parsed.vector_count) * vector_width;
        for (std::size_t i = 0; i < vector_width; ++i)
            parsed.coefficients[destination + i] = vector.coefficients[i];
        bit_offset += vector.bits_consumed;
        if (bit_offset > bits.size() * 8U)
            return Module75AacConfigStatus::input_error;
        ++parsed.vector_count;
    }
    parsed.bits_consumed = bit_offset;
    output = parsed;
    return Module75AacConfigStatus::ok;
}

std::int16_t saturate_module75_aac_q15(const std::int32_t value) noexcept {
    if (value > std::numeric_limits<std::int16_t>::max())
        return std::numeric_limits<std::int16_t>::max();
    if (value < std::numeric_limits<std::int16_t>::min())
        return std::numeric_limits<std::int16_t>::min();
    return static_cast<std::int16_t>(value);
}

std::int32_t multiply_high_module75_aac_q31(
    const std::int32_t left,
    const std::int32_t right) noexcept {
    constexpr std::int64_t scale = std::int64_t{1} << 32;
    const auto product = static_cast<std::int64_t>(left) * right;
    auto high = product / scale;
    if (product < 0 && product % scale != 0) --high;
    return static_cast<std::int32_t>(high);
}

Module75AacConfigStatus zero_fill_module75_aac_spectrum_tail(
    const std::span<std::int32_t> storage,
    const std::size_t first_unused_line,
    const std::size_t remaining_line_count) noexcept {
    if (remaining_line_count > std::numeric_limits<std::size_t>::max() - 3U)
        return Module75AacConfigStatus::invalid_descriptor;
    const auto padded_count = ((remaining_line_count + 3U) / 4U) * 4U;
    if (first_unused_line > storage.size() ||
        padded_count > storage.size() - first_unused_line)
        return Module75AacConfigStatus::input_error;
    std::fill_n(storage.begin() + static_cast<std::ptrdiff_t>(first_unused_line),
                static_cast<std::ptrdiff_t>(padded_count), 0);
    return Module75AacConfigStatus::ok;
}

}  // namespace echo::audio
