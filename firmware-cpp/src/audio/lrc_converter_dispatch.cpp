#include "echo/audio/lrc_converter_dispatch.hpp"

#include "echo/text/firmware_codepage_tables.hpp"

#include <algorithm>

namespace {

constexpr echo::audio::LrcCompatibilityRange kLrcCompatibilityRanges[] = {
    {0xaaa1, 0xaafe, 0xe000}, {0xaba1, 0xabfe, 0xe05e},
    {0xaca1, 0xacfe, 0xe0bc}, {0xada1, 0xadfe, 0xe11a},
    {0xaea1, 0xaefe, 0xe178}, {0xafa1, 0xaffe, 0xe1d6},
    {0xf8a1, 0xf8fe, 0xe234}, {0xf9a1, 0xf9fe, 0xe292},
    {0xfaa1, 0xfafe, 0xe2f0}, {0xfba1, 0xfbfe, 0xe34e},
    {0xfca1, 0xfcfe, 0xe3ac}, {0xfda1, 0xfdfe, 0xe40a},
    {0xfea1, 0xfefe, 0xe468}, {0xa140, 0xa17e, 0xe4c6},
    {0xa180, 0xa1a0, 0xe505}, {0xa240, 0xa27e, 0xe526},
    {0xa280, 0xa2a0, 0xe565}, {0xa340, 0xa37e, 0xe586},
    {0xa380, 0xa3a0, 0xe5c5}, {0xa440, 0xa47e, 0xe5e6},
    {0xa480, 0xa4a0, 0xe625}, {0xa540, 0xa57e, 0xe646},
    {0xa580, 0xa5a0, 0xe685}, {0xa640, 0xa67e, 0xe6a6},
    {0xa680, 0xa6a0, 0xe6e5}, {0xa740, 0xa77e, 0xe706},
    {0xa780, 0xa7a0, 0xe745}, {0xa2ab, 0xa2b0, 0xe766},
    {0xa2e3, 0xa2e4, 0xe76c}, {0xa2ef, 0xa2f0, 0xe76e},
    {0xa2fd, 0xa2fe, 0xe770}, {0xa4f4, 0xa4fe, 0xe772},
    {0xa5f7, 0xa5fe, 0xe77d}, {0xa6b9, 0xa6c0, 0xe785},
    {0xa6d9, 0xa6df, 0xe78d}, {0xa6ed, 0xa6ec, 0xe794},
    {0xa6f6, 0xa6fe, 0xe797}, {0xa7c2, 0xa7d0, 0xe7a0},
    {0xa7f2, 0xa7fe, 0xe7af}, {0xa896, 0xa8a0, 0xe7bc},
    {0xa8c1, 0xa8c4, 0xe7c9}, {0xa8ea, 0xa8fe, 0xe7cd},
    {0xa95d, 0xa95f, 0xe7e4}, {0xa989, 0xa995, 0xe7e7},
    {0xa997, 0xa9a3, 0xe7f4}, {0xa9f0, 0xa9fe, 0xe801},
    {0xd7fa, 0xd7fe, 0xe810}, {0xfe50, 0xfe7e, 0x0259},
    {0xfe80, 0xfea0, 0xe844}, {0xa8bc, 0xa8bc, 0xe7c7},
    {0xa6f3, 0xa6f3, 0xe796}, {0xa8bf, 0xa8bf, 0xe7c8},
    {0xa958, 0xa958, 0xe7e2}, {0xa95b, 0xa95b, 0xe7e3},
};

} // namespace

namespace echo::audio {

LrcConverterPlan plan_lrc_converter(const std::uint16_t encoding_id) noexcept {
    return {
        encoding_id,
        encoding_id == 0 ? LrcConverterEntry::address_030634ac
                         : LrcConverterEntry::address_030632c0,
    };
}

LrcDecodeResult decode_lrc_bytes(
    const std::span<const std::uint8_t> input,
    const LrcDecodeMode mode,
    const std::span<const std::uint8_t> codepage_region,
    const std::span<const std::uint8_t> secondary_fallback_region,
    const std::uint8_t codepage_index,
    const std::size_t max_input_bytes,
    const std::size_t max_output_units) {
    LrcDecodeResult result;
    const auto input_limit = std::min(input.size(), max_input_bytes);
    result.utf16_units.reserve(std::min(input_limit, max_output_units));

    const auto append = [&result, max_output_units](const std::uint16_t value) {
        if (result.utf16_units.size() >= max_output_units) return false;
        result.utf16_units.push_back(value);
        return true;
    };
    const auto map_legacy = [&](const std::uint16_t value, const bool pair) {
        if (codepage_index < 14) {
            const auto mapped = text::lookup_firmware_codepage(
                codepage_region,
                static_cast<text::FirmwareCodepage>(codepage_index),
                value);
            if (mapped) return *mapped;
        }
        if (pair) {
            const auto fallback = lookup_lrc_secondary_fallback(
                secondary_fallback_region, value);
            if (fallback) return *fallback;
        }
        return std::uint16_t{0x3000};
    };

    while (result.bytes_consumed < input_limit) {
        if (result.utf16_units.size() >= max_output_units) {
            result.stop = LrcDecodeStop::output_limit;
            return result;
        }
        const auto first = input[result.bytes_consumed];
        if (first == 0) {
            result.stop = LrcDecodeStop::nul_terminator;
            return result;
        }

        std::uint16_t output = 0;
        std::size_t width = 1;
        if (mode == LrcDecodeMode::utf8) {
            if (first < 0x80) {
                output = first;
            } else if (first < 0xc0) {
                // Firmware routes one-byte high values through its selected
                // runtime map; that dynamic map is exposed separately below.
                result.stop = LrcDecodeStop::unsupported_sequence;
                return result;
            } else if (first < 0xe0) {
                width = 2;
            } else if (first < 0xf0) {
                width = 3;
            } else {
                // The firmware also accepts 4–6-byte forms. Their output
                // stores a 32-bit value and is intentionally left for a
                // later exact model rather than truncated to one UTF-16 unit.
                result.stop = LrcDecodeStop::unsupported_sequence;
                return result;
            }
            if (width > 1) {
                if (input_limit - result.bytes_consumed < width) {
                    result.stop = LrcDecodeStop::truncated_sequence;
                    return result;
                }
                for (std::size_t i = 1; i < width; ++i) {
                    if (input[result.bytes_consumed + i] == 0) {
                        result.stop = LrcDecodeStop::truncated_sequence;
                        return result;
                    }
                }
                if (width == 2) {
                    output = static_cast<std::uint16_t>(
                        ((first & 0x1fU) << 6U) |
                        (input[result.bytes_consumed + 1] & 0x3fU));
                } else {
                    output = static_cast<std::uint16_t>(
                        ((first & 0x0fU) << 12U) |
                        ((input[result.bytes_consumed + 1] & 0x3fU) << 6U) |
                        (input[result.bytes_consumed + 2] & 0x3fU));
                }
            }
        } else if (first < 0x80) {
            output = first;
        } else if (first >= 0x81 && first <= 0xfe &&
                   result.bytes_consumed + 1 < input_limit &&
                   input[result.bytes_consumed + 1] >= 0x40 &&
                   input[result.bytes_consumed + 1] <= 0xfe) {
            width = 2;
            const auto second = input[result.bytes_consumed + 1];
            const auto pair = static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(first) << 8U) | second);
            output = map_legacy(pair, true);
        } else {
            output = map_legacy(first, false);
        }

        if (!append(output)) {
            result.stop = LrcDecodeStop::output_limit;
            return result;
        }
        result.bytes_consumed += width;
    }
    result.stop = LrcDecodeStop::input_limit;
    return result;
}

std::span<const LrcCompatibilityRange> lrc_compatibility_ranges() noexcept {
    return kLrcCompatibilityRanges;
}

std::optional<std::uint16_t> lookup_lrc_compatibility_codepoint(
    const std::uint16_t input) noexcept {
    for (const auto& range : kLrcCompatibilityRanges) {
        if (range.first <= range.last && input >= range.first && input <= range.last) {
            return static_cast<std::uint16_t>(range.output_first + input - range.first);
        }
    }
    return std::nullopt;
}

std::optional<std::uint16_t> lookup_lrc_secondary_fallback(
    const std::span<const std::uint8_t> table,
    const std::uint16_t logical_pair) noexcept {
    constexpr std::uint16_t kFirst = 0xa140;
    constexpr std::uint16_t kLast = 0xf9fe;
    if (logical_pair < kFirst || logical_pair > kLast) return std::nullopt;
    const auto slot = static_cast<std::size_t>(logical_pair - kFirst);
    const auto offset = slot * 2;
    if (offset > table.size() || table.size() - offset < 2) {
        return std::nullopt;
    }
    const auto value = static_cast<std::uint16_t>(table[offset]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(table[offset + 1]) << 8U);
    return value == 0 ? std::optional<std::uint16_t>{0x3000}
                      : std::optional<std::uint16_t>{value};
}

std::optional<LrcRuntimeMapSelection> select_lrc_runtime_map(
    const std::span<const std::uint8_t> firmware_image,
    const std::uint8_t state_selector) noexcept {
    constexpr std::size_t kSelectorTableOffset = 0x45c8;
    constexpr std::size_t kMapBaseOffset = 0x1c45c;
    constexpr std::size_t kMapStride = 0x20000;
    if (kSelectorTableOffset > firmware_image.size() ||
        firmware_image.size() - kSelectorTableOffset <= state_selector) {
        return std::nullopt;
    }
    const auto map_index = firmware_image[kSelectorTableOffset + state_selector];
    const auto map_offset = kMapBaseOffset +
        static_cast<std::size_t>(map_index) * kMapStride;
    if (map_offset > firmware_image.size() ||
        firmware_image.size() - map_offset < kMapStride) {
        return std::nullopt;
    }
    return LrcRuntimeMapSelection{state_selector, map_index, map_offset};
}

std::optional<std::uint16_t> lookup_lrc_runtime_map(
    const std::span<const std::uint8_t> firmware_image,
    const std::uint8_t state_selector,
    const std::uint16_t input_unit) noexcept {
    const auto selection = select_lrc_runtime_map(firmware_image, state_selector);
    if (!selection) return std::nullopt;
    const auto offset = selection->image_offset +
        static_cast<std::size_t>(input_unit) * 2;
    if (offset > firmware_image.size() || firmware_image.size() - offset < 2) {
        return std::nullopt;
    }
    const auto value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(firmware_image[offset]) << 8U) |
        firmware_image[offset + 1]);
    if (input_unit != 0 && value == 0) return std::nullopt;
    return value;
}

} // namespace echo::audio
