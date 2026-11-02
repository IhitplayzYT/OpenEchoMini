#pragma once

#include <cstdint>
#include <optional>

namespace echo::audio {

enum class VorbisHeaderSequenceAction : std::uint8_t {
    parse_packet,
    request_input,
    complete,
    fail,
};

struct VorbisHeaderSequenceStep {
    VorbisHeaderSequenceAction action{};
    std::uint8_t parsed_headers{};
    std::uint8_t required_headers{};
    std::int32_t status{};
};

// FUN_0100c694 parses two headers when its global mode flag is nonzero and
// three when it is zero. A positive packet result enters the format parser; an
// empty result requests more input, and its -1 end/error result maps to -133.
[[nodiscard]] inline std::optional<VorbisHeaderSequenceStep>
plan_vorbis_header_sequence_step(
    bool mode_flag_zero,
    std::uint8_t parsed_headers,
    std::int32_t packet_result,
    std::int32_t packet_parse_status = 0) noexcept {
    const auto required = static_cast<std::uint8_t>(mode_flag_zero ? 3 : 2);
    if (parsed_headers > required) return std::nullopt;
    if (parsed_headers == required) {
        return VorbisHeaderSequenceStep{
            VorbisHeaderSequenceAction::complete, parsed_headers, required, 0};
    }
    if (packet_result == 0) {
        return VorbisHeaderSequenceStep{
            VorbisHeaderSequenceAction::request_input, parsed_headers, required, 0};
    }
    if (packet_result == -1) {
        return VorbisHeaderSequenceStep{
            VorbisHeaderSequenceAction::fail, parsed_headers, required, -133};
    }
    if (packet_result < 0) {
        return VorbisHeaderSequenceStep{
            VorbisHeaderSequenceAction::fail, parsed_headers, required, packet_result};
    }
    if (packet_parse_status != 0) {
        return VorbisHeaderSequenceStep{
            VorbisHeaderSequenceAction::fail, parsed_headers, required,
            packet_parse_status};
    }
    const auto next_count = static_cast<std::uint8_t>(parsed_headers + 1);
    const auto action = next_count == required
        ? VorbisHeaderSequenceAction::complete
        : VorbisHeaderSequenceAction::parse_packet;
    return VorbisHeaderSequenceStep{action, next_count, required, 0};
}

} // namespace echo::audio
