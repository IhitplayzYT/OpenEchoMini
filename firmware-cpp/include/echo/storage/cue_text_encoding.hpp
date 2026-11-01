#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

enum class CueTextEncoding : std::uint8_t {
    default_bytes,
    utf16_little_endian,
    utf16_big_endian,
    utf8,
};

struct CueTextEncodingInfo {
    CueTextEncoding encoding = CueTextEncoding::default_bytes;
    std::size_t bom_size{};
};

// Matches module 38's UTF-16LE/BE and UTF-8 BOM probes. No BOM selects the
// firmware's default byte mode; this helper does not transcode content.
[[nodiscard]] CueTextEncodingInfo detect_cue_text_encoding(
    std::span<const std::uint8_t> bytes) noexcept;

} // namespace echo::storage
