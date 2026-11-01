#pragma once

#include "echo/resources/embedded_image.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace echo::audio {

struct PictureMetadataBlock {
    std::uint32_t picture_type{};
    std::string_view mime_type;
    std::span<const std::uint8_t> description;
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t color_depth{};
    std::uint32_t indexed_colors{};
    std::span<const std::uint8_t> image_data;
    std::optional<resources::EmbeddedImageSignature> signature;
};

// Finds FLAC metadata block type 6 after the four-byte fLaC marker.
[[nodiscard]] std::optional<std::span<const std::uint8_t>>
find_flac_picture_metadata_block(
    std::span<const std::uint8_t> flac_stream) noexcept;

// Parses the decoded FLAC/Vorbis METADATA_BLOCK_PICTURE payload. Every
// returned view refers to the caller's input; a Vorbis base64 wrapper is
// handled separately.
[[nodiscard]] std::optional<PictureMetadataBlock> parse_picture_metadata_block(
    std::span<const std::uint8_t> bytes) noexcept;

} // namespace echo::audio
