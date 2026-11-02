#pragma once

#include "echo/audio/vorbis_packet_mode.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

struct VorbisModeSetup {
    std::vector<VorbisModeConfig> modes;
    bool framing_bit{};
    std::size_t bits_consumed{};
};

namespace detail {

[[nodiscard]] inline bool read_vorbis_mode_bits(
    std::span<const std::uint8_t> bytes, std::size_t& bit_offset,
    std::uint8_t width, std::uint32_t& value) noexcept {
    if (width > 32 || bit_offset > bytes.size() * 8U ||
        width > bytes.size() * 8U - bit_offset) {
        return false;
    }
    value = 0;
    for (std::uint8_t bit = 0; bit < width; ++bit, ++bit_offset) {
        const auto byte_index = bit_offset / 8U;
        const auto bit_index = static_cast<unsigned>(bit_offset % 8U);
        value |= static_cast<std::uint32_t>((bytes[byte_index] >> bit_index) & 1U) << bit;
    }
    return true;
}

} // namespace detail

// Parses the setup-header mode records after mappings. Each record contains a
// block flag, two required-zero 16-bit transform selectors, and an 8-bit
// mapping index, followed by the setup framing bit. The caller may pass the
// shared setup-header bit cursor used by earlier parsers.
[[nodiscard]] inline std::optional<VorbisModeSetup> parse_vorbis_mode_setup(
    std::span<const std::uint8_t> setup_bits,
    std::size_t& bit_offset,
    std::uint16_t mapping_count) {
    if (mapping_count == 0 || mapping_count > 256 ||
        bit_offset > setup_bits.size() * 8U) {
        return std::nullopt;
    }

    const auto start_offset = bit_offset;
    std::uint32_t value = 0;
    if (!detail::read_vorbis_mode_bits(setup_bits, bit_offset, 6, value))
        return std::nullopt;

    VorbisModeSetup setup;
    const auto mode_count = value + 1U;
    setup.modes.reserve(mode_count);
    for (std::uint32_t i = 0; i < mode_count; ++i) {
        std::uint32_t block_flag = 0;
        std::uint32_t window_type = 0;
        std::uint32_t transform_type = 0;
        std::uint32_t mapping_index = 0;
        if (!detail::read_vorbis_mode_bits(setup_bits, bit_offset, 1, block_flag) ||
            !detail::read_vorbis_mode_bits(setup_bits, bit_offset, 16, window_type) ||
            !detail::read_vorbis_mode_bits(setup_bits, bit_offset, 16, transform_type) ||
            !detail::read_vorbis_mode_bits(setup_bits, bit_offset, 8, mapping_index) ||
            window_type != 0 || transform_type != 0 || mapping_index >= mapping_count) {
            return std::nullopt;
        }
        setup.modes.push_back({block_flag != 0,
                               static_cast<std::uint8_t>(mapping_index)});
    }

    std::uint32_t framing = 0;
    if (!detail::read_vorbis_mode_bits(setup_bits, bit_offset, 1, framing) || framing == 0)
        return std::nullopt;
    setup.framing_bit = true;
    setup.bits_consumed = bit_offset - start_offset;
    return setup;
}

} // namespace echo::audio
