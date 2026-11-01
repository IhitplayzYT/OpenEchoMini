#include "echo/audio/module73_layer3_spectral_plan.hpp"

#include "echo/audio/module73_layer3_exponent.hpp"

#include <algorithm>

namespace echo::audio {

std::optional<Module73Layer3LongSpectralPlan>
plan_module73_layer3_long_spectral_bands(
    const unsigned frequency_profile_slot,
    const MpegLayer3GranuleChannel& granule,
    const Module73Layer3ScaleFactors& scalefactors) noexcept {
    if (granule.block_type == 2 || granule.big_values > 288 ||
        scalefactors.value_count < 21) {
        return std::nullopt;
    }
    const auto bands = module73_layer3_band_widths(
        frequency_profile_slot, Module73Layer3BlockShape::long_block);
    if (!bands || bands->widths.size() != 22) return std::nullopt;

    unsigned total_width = 0;
    for (const std::uint8_t width : bands->widths) {
        if ((width & 1U) != 0) return std::nullopt;
        total_width += width;
    }
    if (total_width != 576) return std::nullopt;

    Module73Layer3LongSpectralPlan result{};
    result.big_values_line_count = static_cast<unsigned>(granule.big_values) * 2U;
    result.spectral_line_count = 576;
    unsigned first_line = 0;
    const unsigned second_region_last_band =
        static_cast<unsigned>(granule.region0_count) +
        static_cast<unsigned>(granule.region1_count) + 1U;

    for (unsigned band = 0; band < bands->widths.size(); ++band) {
        unsigned codebook_region = 2;
        if (band <= granule.region0_count) {
            codebook_region = 0;
        } else if (band <= second_region_last_band) {
            codebook_region = 1;
        }
        const unsigned line_count = bands->widths[band];

        const std::uint8_t scalefactor = band < scalefactors.value_count &&
                !scalefactors.intensity_flags[band]
            ? scalefactors.values[band] : 0;
        std::uint8_t preflag_adjustment = 0;
        if (scalefactors.preflag) {
            const auto adjustment = module73_layer3_preflag_adjustment(band);
            if (!adjustment) return std::nullopt;
            preflag_adjustment = *adjustment;
        }
        const auto exponent = module73_layer3_scale_exponent(
            granule.global_gain, scalefactor, granule.scalefac_scale,
            preflag_adjustment);
        if (!exponent) return std::nullopt;

        result.bands[result.band_count++] = {
            static_cast<std::uint16_t>(first_line),
            static_cast<std::uint16_t>(line_count),
            static_cast<std::uint8_t>(band),
            granule.table_select[codebook_region],
            *exponent};
        first_line += line_count;
    }
    return result;
}

std::optional<Module73Layer3ShortSpectralPlan>
plan_module73_layer3_short_spectral_bands(
    const unsigned frequency_profile_slot,
    const MpegLayer3GranuleChannel& granule,
    const Module73Layer3BlockShape shape,
    const Module73Layer3ScaleFactors& scalefactors) noexcept {
    if (granule.block_type != 2 || granule.big_values > 288 ||
        scalefactors.value_count > scalefactors.values.size() ||
        (shape == Module73Layer3BlockShape::short_block && granule.mixed_block) ||
        (shape == Module73Layer3BlockShape::mixed_block && !granule.mixed_block) ||
        shape == Module73Layer3BlockShape::long_block) {
        return std::nullopt;
    }
    const auto table = module73_layer3_band_widths(frequency_profile_slot, shape);
    if (!table || table->widths.empty()) return std::nullopt;

    Module73Layer3ShortSpectralPlan result{};
    result.big_values_line_count = static_cast<unsigned>(granule.big_values) * 2U;
    unsigned table_index = 0;
    unsigned line = 0;
    unsigned scale_index = 0;
    unsigned band_index = 0;

    if (shape == Module73Layer3BlockShape::mixed_block) {
        while (line < 36 && table_index < table->widths.size()) {
            const unsigned width = table->widths[table_index];
            if (width == 0 || line + width > 36 ||
                scale_index >= scalefactors.value_count ||
                result.band_count >= result.bands.size()) {
                return std::nullopt;
            }
            std::uint8_t pretab = 0;
            if (scalefactors.preflag) {
                const auto adjustment = module73_layer3_preflag_adjustment(scale_index);
                if (!adjustment) return std::nullopt;
                pretab = *adjustment;
            }
            const std::uint8_t scale = scalefactors.intensity_flags[scale_index]
                ? 0 : scalefactors.values[scale_index];
            const auto exponent = module73_layer3_scale_exponent(
                granule.global_gain, scale,
                granule.scalefac_scale, pretab);
            if (!exponent) return std::nullopt;
            const std::uint8_t codebook = band_index <= granule.region0_count
                ? granule.table_select[0] : granule.table_select[1];
            result.bands[result.band_count++] = {
                static_cast<std::uint16_t>(line),
                static_cast<std::uint16_t>(width),
                static_cast<std::uint8_t>(scale_index), codebook, *exponent};
            std::fill_n(result.line_exponents.begin() + line, width, *exponent);
            line += width;
            ++table_index;
            ++scale_index;
            ++band_index;
        }
        if (line != 36) return std::nullopt;
        result.mixed_long_band_count = result.band_count;
    }

    while (table_index < table->widths.size()) {
        if (table_index + 2 >= table->widths.size() ||
            result.band_count >= result.bands.size()) {
            return std::nullopt;
        }
        const unsigned width = table->widths[table_index];
        if (width == 0 || table->widths[table_index + 1] != width ||
            table->widths[table_index + 2] != width ||
            line + width * 3U > result.spectral_line_count) {
            return std::nullopt;
        }
        const std::uint8_t codebook = band_index <= granule.region0_count
            ? granule.table_select[0] : granule.table_select[1];
        const unsigned band_first_line = line;
        result.bands[result.band_count++] = {
            static_cast<std::uint16_t>(band_first_line),
            static_cast<std::uint16_t>(width * 3U),
            static_cast<std::uint8_t>(std::min(scale_index, 255U)), codebook, 0};
        for (unsigned window = 0; window < 3; ++window) {
            const unsigned factor_index = scale_index + window;
            const std::uint8_t scalefactor = factor_index < scalefactors.value_count &&
                    !scalefactors.intensity_flags[factor_index]
                ? scalefactors.values[factor_index] : 0;
            const auto exponent = module73_layer3_scale_exponent(
                granule.global_gain, scalefactor, granule.scalefac_scale, 0,
                granule.subblock_gain[window]);
            if (!exponent) return std::nullopt;
            const unsigned window_line = band_first_line + window * width;
            std::fill_n(result.line_exponents.begin() + window_line,
                        width, *exponent);
        }
        line += width * 3U;
        table_index += 3;
        scale_index += 3;
        ++band_index;
    }
    if (line != result.spectral_line_count) return std::nullopt;
    return result;
}

std::optional<Module73Layer3LongPairDecode>
decode_module73_layer3_long_huffman_pairs(
    echo::bitstream::MsbBitReader& bits,
    const Module73Layer3LongSpectralPlan& plan,
    const unsigned huffman_bit_budget) noexcept {
    if (plan.band_count > plan.bands.size() ||
        plan.spectral_line_count != 576 ||
        plan.big_values_line_count > 576 ||
        (plan.big_values_line_count & 1U) != 0) {
        return std::nullopt;
    }

    auto cursor = bits;
    const std::size_t start = cursor.bits_consumed();
    Module73Layer3LongPairDecode result{};
    result.big_values_line_count = plan.big_values_line_count;
    unsigned expected_first_line = 0;
    for (unsigned band_index = 0; band_index < plan.band_count; ++band_index) {
        const auto& band = plan.bands[band_index];
        if (band.first_line != expected_first_line ||
            band.line_count == 0 || (band.line_count & 1U) != 0 ||
            band.codebook_index >= 32 ||
            static_cast<unsigned>(band.first_line) + band.line_count >
                plan.spectral_line_count) {
            return std::nullopt;
        }
        const unsigned decode_lines = band.first_line >= plan.big_values_line_count
            ? 0U : std::min<unsigned>(band.line_count,
                plan.big_values_line_count - band.first_line);
        if ((decode_lines & 1U) != 0) return std::nullopt;
        for (unsigned line = 0; line < decode_lines; line += 2) {
            const auto pair = decode_module73_mpeg_huffman_codebook_pair(
                cursor, band.codebook_index, band.exponent);
            if (!pair) return std::nullopt;
            const std::size_t consumed = cursor.bits_consumed() - start;
            if (consumed > huffman_bit_budget) return std::nullopt;
            const unsigned output_line = band.first_line + line;
            result.values[output_line] = pair->x;
            result.values[output_line + 1] = pair->y;
        }
        expected_first_line += band.line_count;
    }
    if (expected_first_line != 576) return std::nullopt;
    if (plan.big_values_line_count != 0 &&
        expected_first_line < plan.big_values_line_count) return std::nullopt;

    const std::size_t consumed = cursor.bits_consumed() - start;
    if (consumed > 0xffffffffU) return std::nullopt;
    result.huffman_bits_consumed = static_cast<unsigned>(consumed);
    bits = cursor;
    return result;
}

std::optional<Module73Layer3LongSpectralDecode>
decode_module73_layer3_long_spectral_data(
    echo::bitstream::MsbBitReader& bits,
    const Module73Layer3LongSpectralPlan& plan,
    const bool count1_table_select,
    const unsigned huffman_bit_budget) noexcept {
    auto cursor = bits;
    const std::size_t start = cursor.bits_consumed();
    const auto pairs = decode_module73_layer3_long_huffman_pairs(
        cursor, plan, huffman_bit_budget);
    if (!pairs) return std::nullopt;

    Module73Layer3LongSpectralDecode result{};
    result.values = pairs->values;
    result.big_values_line_count = pairs->big_values_line_count;
    unsigned spectral_line = pairs->big_values_line_count;

    while (spectral_line + 4U <= plan.spectral_line_count &&
           cursor.bits_consumed() - start < huffman_bit_budget) {
        std::array<std::int32_t, 4> exponents{};
        for (unsigned value = 0; value < exponents.size(); ++value) {
            const unsigned line = spectral_line + value;
            bool found = false;
            for (unsigned band_index = 0; band_index < plan.band_count;
                 ++band_index) {
                const auto& band = plan.bands[band_index];
                if (line >= band.first_line &&
                    line < static_cast<unsigned>(band.first_line) + band.line_count) {
                    exponents[value] = band.exponent;
                    found = true;
                    break;
                }
            }
            if (!found) return std::nullopt;
        }
        const auto quadruple = decode_module73_mpeg_huffman_count1(
            cursor, count1_table_select ? 1U : 0U, exponents);
        if (!quadruple || quadruple->bits_consumed == 0) return std::nullopt;
        if (cursor.bits_consumed() - start > huffman_bit_budget) return std::nullopt;
        for (unsigned value = 0; value < quadruple->values.size(); ++value) {
            result.values[spectral_line + value] = quadruple->values[value];
        }
        spectral_line += 4;
        result.count1_line_count += 4;
    }

    const std::size_t consumed = cursor.bits_consumed() - start;
    if (consumed > huffman_bit_budget || consumed > 0xffffffffU) {
        return std::nullopt;
    }
    result.huffman_bits_consumed = static_cast<unsigned>(consumed);
    bits = cursor;
    return result;
}

std::optional<Module73Layer3ShortSpectralDecode>
decode_module73_layer3_short_spectral_data(
    echo::bitstream::MsbBitReader& bits,
    const Module73Layer3ShortSpectralPlan& plan,
    const bool count1_table_select,
    const unsigned huffman_bit_budget) noexcept {
    if (plan.band_count > plan.bands.size() ||
        plan.spectral_line_count != 576 || plan.mixed_long_band_count > plan.band_count) {
        return std::nullopt;
    }
    auto cursor = bits;
    const std::size_t start = cursor.bits_consumed();
    Module73Layer3ShortSpectralDecode result{};
    const unsigned big_values_lines = plan.big_values_line_count;
    if (big_values_lines > 576 || (big_values_lines & 1U) != 0)
        return std::nullopt;
    result.big_values_line_count = big_values_lines;
    unsigned expected_first_line = 0;
    for (unsigned band_index = 0; band_index < plan.band_count; ++band_index) {
        const auto& band = plan.bands[band_index];
        if (band.first_line != expected_first_line || band.line_count == 0 ||
            (band.line_count & 1U) != 0 || band.codebook_index >= 32 ||
            static_cast<unsigned>(band.first_line) + band.line_count > 576) {
            return std::nullopt;
        }
        const unsigned decode_lines = band.first_line >= big_values_lines
            ? 0U : std::min<unsigned>(band.line_count,
                big_values_lines - band.first_line);
        if ((decode_lines & 1U) != 0) return std::nullopt;
        for (unsigned line = 0; line < decode_lines; line += 2) {
            const unsigned x_line = band.first_line + line;
            const auto pair = decode_module73_mpeg_huffman_codebook_pair(
                cursor, band.codebook_index,
                plan.line_exponents[x_line], plan.line_exponents[x_line + 1]);
            if (!pair) return std::nullopt;
            if (cursor.bits_consumed() - start > huffman_bit_budget)
                return std::nullopt;
            result.values[x_line] = pair->x;
            result.values[x_line + 1] = pair->y;
        }
        expected_first_line += band.line_count;
    }
    if (expected_first_line != 576) return std::nullopt;

    unsigned spectral_line = big_values_lines;
    while (spectral_line + 4U <= 576 &&
           cursor.bits_consumed() - start < huffman_bit_budget) {
        const std::array<std::int32_t, 4> exponents{
            plan.line_exponents[spectral_line],
            plan.line_exponents[spectral_line + 1],
            plan.line_exponents[spectral_line + 2],
            plan.line_exponents[spectral_line + 3]};
        const auto quadruple = decode_module73_mpeg_huffman_count1(
            cursor, count1_table_select ? 1U : 0U, exponents);
        if (!quadruple || quadruple->bits_consumed == 0) return std::nullopt;
        if (cursor.bits_consumed() - start > huffman_bit_budget)
            return std::nullopt;
        for (unsigned value = 0; value < quadruple->values.size(); ++value) {
            result.values[spectral_line + value] = quadruple->values[value];
        }
        spectral_line += 4;
        result.count1_line_count += 4;
    }
    const std::size_t consumed = cursor.bits_consumed() - start;
    if (consumed > huffman_bit_budget || consumed > 0xffffffffU)
        return std::nullopt;
    result.huffman_bits_consumed = static_cast<unsigned>(consumed);
    bits = cursor;
    return result;
}

} // namespace echo::audio
