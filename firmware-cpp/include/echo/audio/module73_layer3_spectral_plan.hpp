#pragma once

#include "echo/audio/mpeg_layer3_side_info.hpp"
#include "echo/audio/mpeg_huffman_lookup.hpp"
#include "echo/audio/module73_layer3_band_tables.hpp"
#include "echo/audio/module73_layer3_scalefactors.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace echo::audio {

struct Module73Layer3SpectralBandPlan {
    std::uint16_t first_line{};
    std::uint16_t line_count{};
    std::uint8_t scalefactor_band{};
    std::uint8_t codebook_index{};
    std::int32_t exponent{};
};

struct Module73Layer3LongSpectralPlan {
    std::array<Module73Layer3SpectralBandPlan, 22> bands{};
    unsigned band_count{};
    unsigned big_values_line_count{};
    unsigned spectral_line_count{576};
};

struct Module73Layer3ShortSpectralPlan {
    std::array<Module73Layer3SpectralBandPlan, 40> bands{};
    std::array<std::int32_t, 576> line_exponents{};
    unsigned band_count{};
    unsigned mixed_long_band_count{};
    unsigned big_values_line_count{};
    unsigned spectral_line_count{576};
};

struct Module73Layer3LongPairDecode {
    std::array<std::int32_t, 576> values{};
    unsigned big_values_line_count{};
    unsigned huffman_bits_consumed{};
};

struct Module73Layer3LongSpectralDecode {
    std::array<std::int32_t, 576> values{};
    unsigned big_values_line_count{};
    unsigned count1_line_count{};
    unsigned huffman_bits_consumed{};
};

struct Module73Layer3ShortSpectralDecode {
    std::array<std::int32_t, 576> values{};
    unsigned big_values_line_count{};
    unsigned count1_line_count{};
    unsigned huffman_bits_consumed{};
};

// Builds the firmware's long-band Huffman regions up to big_values*2 lines.
// Short/mixed switched-block traversal remains separate and unsupported here.
[[nodiscard]] std::optional<Module73Layer3LongSpectralPlan>
plan_module73_layer3_long_spectral_bands(
    unsigned frequency_profile_slot,
    const MpegLayer3GranuleChannel& granule,
    const Module73Layer3ScaleFactors& scalefactors) noexcept;

// Maps short and mixed blocks into line ranges with one exponent per spectral
// line. Short windows share a Huffman codebook within each scalefactor band,
// while each window retains its own exponent.
[[nodiscard]] std::optional<Module73Layer3ShortSpectralPlan>
plan_module73_layer3_short_spectral_bands(
    unsigned frequency_profile_slot,
    const MpegLayer3GranuleChannel& granule,
    Module73Layer3BlockShape shape,
    const Module73Layer3ScaleFactors& scalefactors) noexcept;

// Decodes the big_values pairs described by a long-band plan. The caller
// passes the remaining part2_3 Huffman budget after scalefactors; failures do
// not advance the input reader. Count1 decoding remains a separate stage.
[[nodiscard]] std::optional<Module73Layer3LongPairDecode>
decode_module73_layer3_long_huffman_pairs(
    echo::bitstream::MsbBitReader& bits,
    const Module73Layer3LongSpectralPlan& plan,
    unsigned huffman_bit_budget) noexcept;

// Decodes the long-block big_values pairs followed by count1 quadruples,
// selecting the exponent for each count1 coefficient from its spectral band.
[[nodiscard]] std::optional<Module73Layer3LongSpectralDecode>
decode_module73_layer3_long_spectral_data(
    echo::bitstream::MsbBitReader& bits,
    const Module73Layer3LongSpectralPlan& plan,
    bool count1_table_select,
    unsigned huffman_bit_budget) noexcept;

[[nodiscard]] std::optional<Module73Layer3ShortSpectralDecode>
decode_module73_layer3_short_spectral_data(
    echo::bitstream::MsbBitReader& bits,
    const Module73Layer3ShortSpectralPlan& plan,
    bool count1_table_select,
    unsigned huffman_bit_budget) noexcept;

} // namespace echo::audio
