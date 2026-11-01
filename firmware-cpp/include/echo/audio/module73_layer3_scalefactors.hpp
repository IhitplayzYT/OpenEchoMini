#pragma once

#include "echo/bitstream/msb_bit_reader.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

enum class Module73Layer3BlockShape : std::uint8_t {
    long_block,
    short_block,
    mixed_block,
};

struct Module73Layer3ScaleFactorWidths {
    std::array<std::uint8_t, 2> groups{};
};

struct Module73LsfScaleFactorLayout {
    std::array<std::uint8_t, 4> widths{};
    std::array<std::uint8_t, 4> counts{};
    bool preflag{};
    bool intensity_stereo{};
};

struct Module73Layer3ScaleFactors {
    std::array<std::uint8_t, 40> values{};
    std::array<bool, 40> intensity_flags{};
    unsigned value_count{};
    unsigned bits_consumed{};
    bool preflag{};
};

// MPEG-1's 16-entry slen1/slen2 table embedded at 0x01024974.
[[nodiscard]] std::optional<Module73Layer3ScaleFactorWidths>
module73_mpeg1_scalefactor_widths(unsigned scalefac_compress) noexcept;

// Decodes the module 73 MPEG-2/2.5 scalefac_compress mapping and its four
// scalefactor group counts. Intensity mode is for the right channel when
// intensity stereo is active; table traversal and bit consumption remain
// separate operations.
[[nodiscard]] std::optional<Module73LsfScaleFactorLayout>
module73_lsf_scalefactor_layout(
    unsigned scalefac_compress,
    Module73Layer3BlockShape shape,
    bool intensity_stereo_right_channel) noexcept;

// Decodes MPEG-1 long, short, or mixed scalefactors. For a long second
// granule, scfsi-selected groups are copied from previous_granule_values.
[[nodiscard]] std::optional<Module73Layer3ScaleFactors>
decode_module73_mpeg1_scalefactors(
    echo::bitstream::MsbBitReader& bits,
    unsigned scalefac_compress,
    Module73Layer3BlockShape shape,
    bool preflag,
    std::array<bool, 4> scfsi = {},
    std::span<const std::uint8_t> previous_granule_values = {}) noexcept;

// Decodes LSF groups according to the recovered width/count layout. Intensity
// sentinel flags are retained separately from decoded values.
[[nodiscard]] std::optional<Module73Layer3ScaleFactors>
decode_module73_lsf_scalefactors(
    echo::bitstream::MsbBitReader& bits,
    const Module73LsfScaleFactorLayout& layout) noexcept;

} // namespace echo::audio
