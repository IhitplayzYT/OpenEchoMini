#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

enum class MpegVersion : std::uint8_t { v1, v2, v2_5 };
enum class MpegLayer : std::uint8_t { layer1, layer2, layer3 };

struct MpegFrameHeader {
    MpegVersion version{};
    MpegLayer layer{};
    std::uint8_t bitrate_index{}; // 0 denotes free format; 15 is reserved.
    std::uint8_t sample_rate_index{};
    std::uint32_t sample_rate_hz{};
    std::uint8_t channel_mode{}; // MPEG values 0..3; 3 is mono.
    std::uint8_t mode_extension{};
    bool has_crc{};
    bool padded{};
};

// Parses the four-byte MPEG audio header fields validated in module 15.
// It does not resolve bitrate tables, frame lengths, or decode audio data.
[[nodiscard]] std::optional<MpegFrameHeader> parse_mpeg_frame_header(
    std::span<const std::uint8_t> bytes) noexcept;

// Finds the next sync candidate while retaining the module's observed
// eight-byte minimum tail from the candidate position.
[[nodiscard]] std::optional<std::size_t> find_mpeg_sync(
    std::span<const std::uint8_t> bytes,
    std::size_t start_offset = 0) noexcept;

} // namespace echo::audio
