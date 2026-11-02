#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace echo::audio {

struct VorbisCommentHeader {
    std::vector<std::uint8_t> vendor;
    std::vector<std::vector<std::uint8_t>> comments;
    std::size_t bits_consumed{};
};

// Parses a complete type-3 Vorbis comment packet. Comment and vendor payloads
// are retained as bytes so callers can choose their own text encoding policy.
[[nodiscard]] inline std::optional<VorbisCommentHeader>
parse_vorbis_comment_header(std::span<const std::uint8_t> packet) {
    constexpr std::uint8_t signature[]{'v', 'o', 'r', 'b', 'i', 's'};
    if (packet.size() < 7U || packet[0] != 3) return std::nullopt;
    for (std::size_t i = 0; i < 6U; ++i) {
        if (packet[i + 1U] != signature[i]) return std::nullopt;
    }

    std::size_t cursor = 7U;
    const auto read_u32 = [&](std::uint32_t& value) noexcept {
        if (cursor > packet.size() || packet.size() - cursor < 4U) return false;
        value = static_cast<std::uint32_t>(packet[cursor]) |
                (static_cast<std::uint32_t>(packet[cursor + 1U]) << 8U) |
                (static_cast<std::uint32_t>(packet[cursor + 2U]) << 16U) |
                (static_cast<std::uint32_t>(packet[cursor + 3U]) << 24U);
        cursor += 4U;
        return true;
    };
    const auto take = [&](std::uint32_t length,
                          std::vector<std::uint8_t>& output) {
        if (cursor > packet.size() || length > packet.size() - cursor) return false;
        output.assign(packet.begin() + static_cast<std::ptrdiff_t>(cursor),
                      packet.begin() + static_cast<std::ptrdiff_t>(cursor + length));
        cursor += length;
        return true;
    };

    VorbisCommentHeader result;
    std::uint32_t vendor_length = 0;
    std::uint32_t comment_count = 0;
    if (!read_u32(vendor_length) || !take(vendor_length, result.vendor) ||
        !read_u32(comment_count) ||
        comment_count > (packet.size() - cursor) / 4U) {
        return std::nullopt;
    }
    result.comments.reserve(comment_count);
    for (std::uint32_t i = 0; i < comment_count; ++i) {
        std::uint32_t length = 0;
        std::vector<std::uint8_t> comment;
        if (!read_u32(length) || !take(length, comment)) return std::nullopt;
        result.comments.push_back(std::move(comment));
    }
    if (cursor >= packet.size() || (packet[cursor] & 1U) == 0) return std::nullopt;
    result.bits_consumed = cursor * 8U + 1U;
    return result;
}

} // namespace echo::audio
