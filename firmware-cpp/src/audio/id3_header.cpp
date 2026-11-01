#include "echo/audio/id3_header.hpp"

#include <cstddef>

namespace echo::audio {

std::optional<Id3Header> parse_id3_header(
    std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < 10 || bytes[0] != 'I' || bytes[1] != 'D' || bytes[2] != '3') {
        return std::nullopt;
    }

    const auto version = bytes[3];
    if (version < 2 || version > 4) return std::nullopt;

    std::uint32_t size = 0;
    for (std::size_t i = 6; i < 10; ++i) {
        if ((bytes[i] & 0x80U) != 0) return std::nullopt;
        size = (size << 7U) | bytes[i];
    }

    return Id3Header{version, bytes[4], bytes[5], size};
}

} // namespace echo::audio
