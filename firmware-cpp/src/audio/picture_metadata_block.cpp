#include "echo/audio/picture_metadata_block.hpp"

#include <algorithm>
#include <array>

namespace echo::audio {
namespace {

bool read_u32_be(std::span<const std::uint8_t> bytes,
                 std::size_t& cursor,
                 std::uint32_t& value) noexcept {
    if (cursor > bytes.size() || bytes.size() - cursor < 4) return false;
    value = (static_cast<std::uint32_t>(bytes[cursor]) << 24U) |
            (static_cast<std::uint32_t>(bytes[cursor + 1]) << 16U) |
            (static_cast<std::uint32_t>(bytes[cursor + 2]) << 8U) |
            static_cast<std::uint32_t>(bytes[cursor + 3]);
    cursor += 4;
    return true;
}

bool take(std::span<const std::uint8_t> bytes,
          std::size_t& cursor,
          std::uint32_t length,
          std::span<const std::uint8_t>& result) noexcept {
    if (cursor > bytes.size() || length > bytes.size() - cursor) return false;
    result = bytes.subspan(cursor, length);
    cursor += length;
    return true;
}

} // namespace

std::optional<std::span<const std::uint8_t>> find_flac_picture_metadata_block(
    std::span<const std::uint8_t> flac_stream) noexcept {
    constexpr std::array<std::uint8_t, 4> marker{'f', 'L', 'a', 'C'};
    if (flac_stream.size() < marker.size() ||
        !std::equal(marker.begin(), marker.end(), flac_stream.begin())) {
        return std::nullopt;
    }

    std::size_t cursor = marker.size();
    while (cursor <= flac_stream.size() && flac_stream.size() - cursor >= 4) {
        const auto header = flac_stream.subspan(cursor, 4);
        const bool last_block = (header[0] & 0x80U) != 0;
        const auto block_type = static_cast<std::uint8_t>(header[0] & 0x7fU);
        const auto block_size = (static_cast<std::uint32_t>(header[1]) << 16U) |
                               (static_cast<std::uint32_t>(header[2]) << 8U) |
                               static_cast<std::uint32_t>(header[3]);
        cursor += 4;
        if (block_size > flac_stream.size() - cursor) return std::nullopt;
        if (block_type == 6) return flac_stream.subspan(cursor, block_size);
        cursor += block_size;
        if (last_block) return std::nullopt;
    }
    return std::nullopt;
}

std::optional<PictureMetadataBlock> parse_picture_metadata_block(
    std::span<const std::uint8_t> bytes) noexcept {
    std::size_t cursor = 0;
    PictureMetadataBlock result;
    std::uint32_t mime_size = 0;
    std::uint32_t description_size = 0;
    std::uint32_t image_size = 0;
    std::span<const std::uint8_t> mime;

    if (!read_u32_be(bytes, cursor, result.picture_type) ||
        !read_u32_be(bytes, cursor, mime_size) ||
        !take(bytes, cursor, mime_size, mime) ||
        !read_u32_be(bytes, cursor, description_size) ||
        !take(bytes, cursor, description_size, result.description) ||
        !read_u32_be(bytes, cursor, result.width) ||
        !read_u32_be(bytes, cursor, result.height) ||
        !read_u32_be(bytes, cursor, result.color_depth) ||
        !read_u32_be(bytes, cursor, result.indexed_colors) ||
        !read_u32_be(bytes, cursor, image_size) ||
        !take(bytes, cursor, image_size, result.image_data)) {
        return std::nullopt;
    }

    result.mime_type = mime.empty()
        ? std::string_view{}
        : std::string_view(reinterpret_cast<const char*>(mime.data()), mime.size());
    result.signature = resources::find_embedded_image(result.image_data);
    return result;
}

} // namespace echo::audio
