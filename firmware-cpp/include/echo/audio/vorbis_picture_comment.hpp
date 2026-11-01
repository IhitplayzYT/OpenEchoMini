#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

// `comment_payload` begins with the Vorbis vendor length field, after any
// Ogg packet prefix. Returned bytes borrow from the caller's input.
[[nodiscard]] std::optional<std::span<const std::uint8_t>>
find_vorbis_picture_base64_comment(
    std::span<const std::uint8_t> comment_payload) noexcept;

[[nodiscard]] std::optional<std::vector<std::uint8_t>>
decode_module18_base64(std::span<const std::uint8_t> encoded);

[[nodiscard]] std::optional<std::vector<std::uint8_t>>
decode_vorbis_picture_comment(
    std::span<const std::uint8_t> comment_payload);

// Reassembles packets from one Ogg logical stream, then decodes the type-3
// Vorbis comment packet's METADATA_BLOCK_PICTURE value. CRCs and chained
// logical-stream selection are outside this bounded reconstruction.
[[nodiscard]] std::optional<std::vector<std::uint8_t>>
decode_ogg_vorbis_picture_comment(std::span<const std::uint8_t> ogg_stream);

} // namespace echo::audio
