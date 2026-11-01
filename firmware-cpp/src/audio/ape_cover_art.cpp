#include "echo/audio/ape_cover_art.hpp"

#include <algorithm>
#include <cstddef>

namespace echo::audio {
namespace {

std::uint32_t read_u32_le(std::span<const std::uint8_t> bytes,
                          std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

} // namespace

std::optional<ApeTagView> module18_locate_ape_tag(
    std::span<const std::uint8_t> file) noexcept {
    constexpr std::size_t footer_size = 32;
    constexpr std::string_view signature = "APETAGEX";
    if (file.size() < footer_size) return std::nullopt;

    const auto footer_offset = file.size() - footer_size;
    const auto footer = file.subspan(footer_offset, footer_size);
    for (std::size_t i = 0; i < signature.size(); ++i) {
        if (footer[i] != static_cast<std::uint8_t>(signature[i])) return std::nullopt;
    }

    const auto version = read_u32_le(footer, 8);
    const auto tag_size = read_u32_le(footer, 12);
    const auto item_count = read_u32_le(footer, 16);
    const auto flags = read_u32_le(footer, 20);
    if (item_count > 0x10000U || (flags & 0x20000000U) != 0 ||
        tag_size > file.size() || tag_size < footer_size) {
        return std::nullopt;
    }

    const auto tag_start = file.size() - tag_size;
    const auto items_end = file.size() - footer_size;
    if (tag_start > items_end) return std::nullopt;
    return ApeTagView{version, tag_size, item_count, flags,
                      file.subspan(tag_start, items_end - tag_start)};
}

std::optional<ApeCoverArtItem> find_ape_front_cover_item(
    std::span<const std::uint8_t> items,
    std::uint32_t item_count) noexcept {
    constexpr std::string_view cover_key = "Cover Art (front)";
    std::size_t cursor = 0;

    for (std::uint32_t item = 0; item < item_count; ++item) {
        if (cursor > items.size() || items.size() - cursor < 8) return std::nullopt;
        const auto value_size = read_u32_le(items, cursor);
        const auto flags = read_u32_le(items, cursor + 4);
        cursor += 8;

        const auto key_begin = cursor;
        const auto remaining = items.subspan(cursor);
        const auto key_end = std::find(remaining.begin(), remaining.end(), 0);
        if (key_end == remaining.end()) return std::nullopt;
        const auto key_length = static_cast<std::size_t>(key_end - remaining.begin());
        const std::string_view key(
            reinterpret_cast<const char*>(items.data() + key_begin), key_length);
        cursor += key_length + 1;
        if (value_size > items.size() - cursor) return std::nullopt;

        const auto value = items.subspan(cursor, value_size);
        if (key == cover_key) {
            const auto filename_end = std::find(value.begin(), value.end(), 0);
            if (filename_end == value.end()) return std::nullopt;
            const auto filename_size = static_cast<std::size_t>(filename_end - value.begin());
            const std::string_view filename(
                reinterpret_cast<const char*>(value.data()), filename_size);
            const auto image_offset = filename_size + 1;
            const auto image = value.subspan(image_offset);
            return ApeCoverArtItem{flags, filename, image,
                                   resources::find_embedded_image(image)};
        }

        cursor += value_size;
    }
    return std::nullopt;
}

} // namespace echo::audio
