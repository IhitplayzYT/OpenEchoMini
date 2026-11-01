#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct Id3Header {
    std::uint8_t major_version{};
    std::uint8_t revision{};
    std::uint8_t flags{};
    std::uint32_t tag_size{};
};

// Parses the observed 10-byte ID3v2 header form (versions 2 through 4).
// Invalid signatures, unsupported versions, and non-synchsafe size bytes fail.
[[nodiscard]] std::optional<Id3Header> parse_id3_header(
    std::span<const std::uint8_t> bytes) noexcept;

} // namespace echo::audio
