#include "echo/audio/vorbis_packet_mode.hpp"

namespace echo::audio {
namespace {

bool read_lsb(std::span<const std::uint8_t> bytes,
              std::size_t& bit_offset,
              std::uint8_t count,
              std::uint32_t& value) noexcept {
    if (count > 32) return false;
    value = 0;
    for (std::uint8_t bit = 0; bit < count; ++bit) {
        const auto byte_index = bit_offset / 8;
        if (byte_index >= bytes.size()) return false;
        const auto shift = static_cast<unsigned>(bit_offset % 8);
        value |= static_cast<std::uint32_t>((bytes[byte_index] >> shift) & 1U) << bit;
        ++bit_offset;
    }
    return true;
}

std::uint8_t ilog_width(std::uint32_t value) noexcept {
    std::uint8_t width = 0;
    while (value != 0) {
        ++width;
        value >>= 1U;
    }
    return width;
}

} // namespace

std::optional<VorbisPacketMode> decode_vorbis_packet_mode(
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    std::span<const VorbisModeConfig> modes,
    std::uint16_t mapping_count) {
    if (modes.empty() || mapping_count == 0) return std::nullopt;

    const auto original_offset = bit_offset;
    auto cursor = bit_offset;
    std::uint32_t packet_type = 0;
    if (!read_lsb(packet, cursor, 1, packet_type) || packet_type != 0) {
        return std::nullopt;
    }

    std::uint32_t mode_index = 0;
    const auto mode_width = ilog_width(static_cast<std::uint32_t>(modes.size() - 1));
    if (!read_lsb(packet, cursor, mode_width, mode_index) || mode_index >= modes.size()) {
        return std::nullopt;
    }
    const auto& mode = modes[mode_index];
    if (mode.mapping_index >= mapping_count) return std::nullopt;

    VorbisPacketMode result;
    result.mode_index = mode_index;
    result.mapping_index = mode.mapping_index;
    result.long_block = mode.long_block;
    if (mode.long_block) {
        std::uint32_t previous = 0;
        std::uint32_t next = 0;
        if (!read_lsb(packet, cursor, 1, previous) ||
            !read_lsb(packet, cursor, 1, next)) {
            return std::nullopt;
        }
        result.previous_window_long = previous != 0;
        result.next_window_long = next != 0;
    }
    result.bits_consumed = cursor - original_offset;
    bit_offset = cursor;
    return result;
}

} // namespace echo::audio
