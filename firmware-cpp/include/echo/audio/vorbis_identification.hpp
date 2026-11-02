#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct VorbisIdentificationHeader {
    std::uint32_t version{};
    std::uint8_t channels{};
    std::uint32_t sample_rate{};
    std::uint32_t bitrate_maximum{};
    std::uint32_t bitrate_nominal{};
    std::uint32_t bitrate_minimum{};
    std::uint8_t small_block_exponent{};
    std::uint8_t large_block_exponent{};
    std::uint32_t small_block_size{};
    std::uint32_t large_block_size{};
};

inline constexpr std::size_t kVorbisIdentificationHeaderBytes = 30;

// Parses the packet prefix and fixed identification fields read by
// FUN_010097b0. Integer fields are little-endian; the block exponents share
// the low/high nibbles of byte 28, and the framing bit is bit zero of byte 29.
[[nodiscard]] inline std::optional<VorbisIdentificationHeader>
parse_vorbis_identification_header(
    std::span<const std::uint8_t> packet) noexcept {
    constexpr std::array<std::uint8_t, 6> kSignature{
        'v', 'o', 'r', 'b', 'i', 's'};
    if (packet.size() < kVorbisIdentificationHeaderBytes || packet[0] != 1) {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < kSignature.size(); ++i) {
        if (packet[i + 1] != kSignature[i]) return std::nullopt;
    }
    if ((packet[29] & 1U) == 0) return std::nullopt;

    const auto read_u32_le = [&](std::size_t offset) noexcept {
        return static_cast<std::uint32_t>(packet[offset]) |
            (static_cast<std::uint32_t>(packet[offset + 1]) << 8U) |
            (static_cast<std::uint32_t>(packet[offset + 2]) << 16U) |
            (static_cast<std::uint32_t>(packet[offset + 3]) << 24U);
    };
    VorbisIdentificationHeader header{};
    header.version = read_u32_le(7);
    header.channels = packet[11];
    header.sample_rate = read_u32_le(12);
    header.bitrate_maximum = read_u32_le(16);
    header.bitrate_nominal = read_u32_le(20);
    header.bitrate_minimum = read_u32_le(24);
    header.small_block_exponent = packet[28] & 0x0fU;
    header.large_block_exponent = packet[28] >> 4U;
    header.small_block_size = 1U << header.small_block_exponent;
    header.large_block_size = 1U << header.large_block_exponent;

    // The firmware explicitly rejects nonzero versions. Zero channel/rate and
    // malformed framing are rejected here as conservative packet validation.
    if (header.version != 0 || header.channels == 0 || header.sample_rate == 0) {
        return std::nullopt;
    }
    return header;
}

} // namespace echo::audio
