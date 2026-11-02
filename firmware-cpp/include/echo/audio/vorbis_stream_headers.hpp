#pragma once

#include "echo/audio/vorbis_comment_header.hpp"
#include "echo/audio/vorbis_identification.hpp"
#include "echo/audio/vorbis_setup_header.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

namespace echo::audio {

struct VorbisStreamHeaders {
    VorbisIdentificationHeader identification;
    std::optional<VorbisCommentHeader> comments;
    VorbisSetupHeader setup;
    std::uint8_t packet_count{};
};

// Parses the header sequence selected by FUN_0100c694: two packets when the
// mode flag is nonzero (identification + setup), or three when it is zero
// (identification + comment + setup).
[[nodiscard]] inline std::optional<VorbisStreamHeaders>
parse_vorbis_stream_headers(
    std::span<const std::span<const std::uint8_t>> packets,
    bool mode_flag_zero) {
    const auto expected_count = static_cast<std::size_t>(mode_flag_zero ? 3U : 2U);
    if (packets.size() != expected_count) return std::nullopt;

    const auto identification = parse_vorbis_identification_header(packets.front());
    if (!identification) return std::nullopt;

    VorbisStreamHeaders headers;
    headers.identification = *identification;
    headers.packet_count = static_cast<std::uint8_t>(packets.size());
    const auto setup_packet_index = mode_flag_zero ? 2U : 1U;
    if (mode_flag_zero) {
        auto comments = parse_vorbis_comment_header(packets[1]);
        if (!comments) return std::nullopt;
        headers.comments = std::move(*comments);
    }
    auto setup = parse_vorbis_setup_header(
        packets[setup_packet_index], headers.identification.channels);
    if (!setup) return std::nullopt;
    headers.setup = std::move(*setup);
    return headers;
}

} // namespace echo::audio
