#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace echo::audio {

// Display text emitted by module 27's playback information formatter.
[[nodiscard]] std::string_view playback_format_label(std::uint16_t codec_id) noexcept;

struct PlaybackFormatDisplay {
    std::string text;
    std::uint16_t quality_icon_id{};
    bool dsd{};
};

// Reconstructs the visible metadata text and quality-icon selection from
// module 27's FUN_0307c674 / FUN_0307d3d0. Bitrate is supplied in bits/second,
// sample rate in hertz, and bit_depth is the firmware's raw format field.
[[nodiscard]] PlaybackFormatDisplay format_playback_information(
    std::uint16_t codec_id, std::uint32_t bitrate_bits_per_second,
    std::uint32_t sample_rate_hz, std::uint32_t bit_depth);

} // namespace echo::audio
