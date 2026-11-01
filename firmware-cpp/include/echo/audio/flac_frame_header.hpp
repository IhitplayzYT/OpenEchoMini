#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

enum class FlacChannelAssignment : std::uint8_t {
    independent_mono = 0,
    independent_stereo = 1,
    left_side_stereo = 8,
    side_right_stereo = 9,
    mid_side_stereo = 10,
};

struct FlacFrameHeader {
    std::uint32_t block_size{};
    std::uint32_t sample_rate{};
    std::uint8_t channels{};
    std::uint8_t bits_per_sample{};
    FlacChannelAssignment channel_assignment{};
    std::uint64_t frame_number{};
    std::uint8_t header_crc{};
    std::size_t encoded_size{};
};

struct FlacStreamDefaults {
    std::optional<std::uint32_t> sample_rate;
    std::optional<std::uint8_t> bits_per_sample;
};

// Parses the module 78 fixed-block FLAC header subset. The target scanner
// searches for FF F8, limits blocks to 16..4608 samples, and accepts mono,
// stereo, and the three stereo decorrelation assignments.
[[nodiscard]] std::optional<FlacFrameHeader> parse_flac_frame_header(
    std::span<const std::uint8_t> bytes,
    FlacStreamDefaults defaults = {},
    bool verify_crc = true) noexcept;

} // namespace echo::audio
