#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct VorbisModeConfig {
    bool long_block{};
    std::uint8_t mapping_index{};
};

struct VorbisPacketMode {
    std::uint32_t mode_index{};
    std::uint8_t mapping_index{};
    bool long_block{};
    bool previous_window_long{};
    bool next_window_long{};
    std::size_t bits_consumed{};
};

// Reads the audio-packet type, mode number, and long-block window flags.
// Packet bytes use the firmware's least-significant-bit-first field order.
[[nodiscard]] std::optional<VorbisPacketMode> decode_vorbis_packet_mode(
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    std::span<const VorbisModeConfig> modes,
    std::uint16_t mapping_count);

} // namespace echo::audio
