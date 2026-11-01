#include "echo/audio/module73_layer3_scalefactors.hpp"

#include <array>

namespace echo::audio {

namespace {

constexpr std::array<Module73Layer3ScaleFactorWidths, 16> kMpeg1Widths{{
    {{{0, 0}}}, {{{0, 1}}}, {{{0, 2}}}, {{{0, 3}}},
    {{{3, 0}}}, {{{1, 1}}}, {{{1, 2}}}, {{{1, 3}}},
    {{{2, 1}}}, {{{2, 2}}}, {{{2, 3}}}, {{{3, 1}}},
    {{{3, 2}}}, {{{3, 3}}}, {{{4, 2}}}, {{{4, 3}}},
}};

// Nine four-group count rows at 0x01024994. Rows are selected by compression
// class and then by long, short, or mixed block shape.
constexpr std::array<std::array<std::uint8_t, 4>, 18> kLsfGroupCounts{{
    {{6, 5, 5, 5}}, {{9, 9, 9, 9}}, {{6, 9, 9, 9}},
    {{6, 5, 7, 3}}, {{9, 9, 12, 6}}, {{6, 9, 12, 6}},
    {{11, 10, 0, 0}}, {{18, 18, 0, 0}}, {{15, 18, 0, 0}},
    {{7, 7, 7, 0}}, {{12, 12, 12, 0}}, {{6, 15, 12, 0}},
    {{6, 6, 6, 3}}, {{12, 9, 9, 6}}, {{6, 12, 9, 6}},
    {{8, 8, 5, 0}}, {{15, 12, 9, 0}}, {{6, 18, 9, 0}},
}};

std::optional<unsigned> shape_index(const Module73Layer3BlockShape shape) noexcept {
    switch (shape) {
    case Module73Layer3BlockShape::long_block: return 0;
    case Module73Layer3BlockShape::short_block: return 1;
    case Module73Layer3BlockShape::mixed_block: return 2;
    }
    return std::nullopt;
}

} // namespace

std::optional<Module73Layer3ScaleFactorWidths>
module73_mpeg1_scalefactor_widths(const unsigned scalefac_compress) noexcept {
    if (scalefac_compress >= kMpeg1Widths.size()) return std::nullopt;
    return kMpeg1Widths[scalefac_compress];
}

std::optional<Module73LsfScaleFactorLayout> module73_lsf_scalefactor_layout(
    const unsigned scalefac_compress,
    const Module73Layer3BlockShape shape,
    const bool intensity_stereo_right_channel) noexcept {
    if (scalefac_compress > 511) return std::nullopt;
    const auto shape_row = shape_index(shape);
    if (!shape_row) return std::nullopt;

    Module73LsfScaleFactorLayout result{};
    result.intensity_stereo = intensity_stereo_right_channel;
    unsigned row_group{};

    if (!intensity_stereo_right_channel) {
        if (scalefac_compress < 400) {
            const unsigned high = scalefac_compress >> 4U;
            result.widths = {static_cast<std::uint8_t>(high / 5U),
                             static_cast<std::uint8_t>(high % 5U),
                             static_cast<std::uint8_t>((scalefac_compress >> 2U) & 3U),
                             static_cast<std::uint8_t>(scalefac_compress & 3U)};
            row_group = 0;
        } else if (scalefac_compress < 500) {
            const unsigned adjusted = scalefac_compress - 400U;
            const unsigned high = adjusted >> 2U;
            result.widths = {static_cast<std::uint8_t>(high / 5U),
                             static_cast<std::uint8_t>(high % 5U),
                             static_cast<std::uint8_t>(adjusted & 3U), 0};
            row_group = 3;
        } else {
            const unsigned adjusted = scalefac_compress - 500U;
            result.widths = {static_cast<std::uint8_t>(adjusted / 3U),
                             static_cast<std::uint8_t>(adjusted % 3U), 0, 0};
            result.preflag = true;
            row_group = 6;
        }
    } else {
        const unsigned compressed = scalefac_compress >> 1U;
        if (compressed < 180) {
            const unsigned rem36 = compressed % 36U;
            result.widths = {static_cast<std::uint8_t>(compressed / 36U),
                             static_cast<std::uint8_t>(rem36 / 6U),
                             static_cast<std::uint8_t>(rem36 % 6U), 0};
            row_group = 9;
        } else if (compressed < 244) {
            const unsigned adjusted = compressed - 180U;
            result.widths = {static_cast<std::uint8_t>((adjusted & 0x3fU) >> 4U),
                             static_cast<std::uint8_t>((adjusted & 0x0fU) >> 2U),
                             static_cast<std::uint8_t>(adjusted & 3U), 0};
            row_group = 12;
        } else {
            const unsigned adjusted = compressed - 244U;
            result.widths = {static_cast<std::uint8_t>(adjusted / 3U),
                             static_cast<std::uint8_t>(adjusted % 3U), 0, 0};
            row_group = 15;
        }
    }

    const unsigned row = row_group + *shape_row;
    if (row >= kLsfGroupCounts.size()) return std::nullopt;
    result.counts = kLsfGroupCounts[row];
    return result;
}

std::optional<Module73Layer3ScaleFactors> decode_module73_mpeg1_scalefactors(
    echo::bitstream::MsbBitReader& bits,
    const unsigned scalefac_compress,
    const Module73Layer3BlockShape shape,
    const bool preflag,
    const std::array<bool, 4> scfsi,
    const std::span<const std::uint8_t> previous_granule_values) noexcept {
    const auto widths = module73_mpeg1_scalefactor_widths(scalefac_compress);
    const auto block_shape = shape_index(shape);
    if (!widths || !block_shape) return std::nullopt;

    std::array<unsigned, 4> counts{};
    std::array<unsigned, 4> group_widths{};
    if (*block_shape == 0) {
        counts = {6, 5, 5, 5};
        group_widths = {widths->groups[0], widths->groups[0],
                        widths->groups[1], widths->groups[1]};
        if ((scfsi[0] || scfsi[1] || scfsi[2] || scfsi[3]) &&
            previous_granule_values.size() < 21) {
            return std::nullopt;
        }
    } else if (*block_shape == 1) {
        counts = {18, 18, 0, 0};
        group_widths = {widths->groups[0], widths->groups[1], 0, 0};
    } else {
        counts = {17, 18, 0, 0};
        group_widths = {widths->groups[0], widths->groups[1], 0, 0};
    }

    auto cursor = bits;
    const std::size_t start = cursor.bits_consumed();
    Module73Layer3ScaleFactors result{};
    result.preflag = preflag;
    unsigned output = 0;
    for (unsigned group = 0; group < counts.size(); ++group) {
        for (unsigned i = 0; i < counts[group]; ++i) {
            if (*block_shape == 0 && scfsi[group]) {
                result.values[output] = previous_granule_values[output];
            } else if (group_widths[group] != 0) {
                const auto value = cursor.read(group_widths[group]);
                if (!value) return std::nullopt;
                result.values[output] = static_cast<std::uint8_t>(*value);
            }
            ++output;
        }
    }
    const std::size_t consumed = cursor.bits_consumed() - start;
    if (consumed > 0xffffffffU) return std::nullopt;
    result.value_count = output;
    result.bits_consumed = static_cast<unsigned>(consumed);
    bits = cursor;
    return result;
}

std::optional<Module73Layer3ScaleFactors> decode_module73_lsf_scalefactors(
    echo::bitstream::MsbBitReader& bits,
    const Module73LsfScaleFactorLayout& layout) noexcept {
    unsigned total = 0;
    for (unsigned group = 0; group < layout.counts.size(); ++group) {
        if (layout.widths[group] > 16) return std::nullopt;
        total += layout.counts[group];
    }
    if (total > 40) return std::nullopt;

    auto cursor = bits;
    const std::size_t start = cursor.bits_consumed();
    Module73Layer3ScaleFactors result{};
    result.preflag = layout.preflag;
    for (unsigned group = 0; group < layout.counts.size(); ++group) {
        const unsigned width = layout.widths[group];
        const std::uint32_t sentinel = width == 0
            ? 0U : (std::uint32_t{1} << width) - 1U;
        for (unsigned i = 0; i < layout.counts[group]; ++i) {
            std::uint32_t value = 0;
            if (width != 0) {
                const auto decoded = cursor.read(width);
                if (!decoded) return std::nullopt;
                value = *decoded;
            }
            result.values[result.value_count] = static_cast<std::uint8_t>(value);
            result.intensity_flags[result.value_count] =
                layout.intensity_stereo && value == sentinel;
            ++result.value_count;
        }
    }
    const std::size_t consumed = cursor.bits_consumed() - start;
    if (consumed > 0xffffffffU) return std::nullopt;
    result.bits_consumed = static_cast<unsigned>(consumed);
    bits = cursor;
    return result;
}

} // namespace echo::audio
