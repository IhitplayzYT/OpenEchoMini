#include "echo/audio/module33_records.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>

namespace echo::audio {
namespace {

constexpr std::array<std::uint8_t, 16> kAsfHeaderObjectGuid{
    0x30, 0x26, 0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11,
    0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c,
};
constexpr std::array<std::uint8_t, 16> kContentDescriptionGuid{
    0x33, 0x26, 0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11,
    0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c,
};
constexpr std::array<std::uint8_t, 16> kExtendedContentDescriptionGuid{
    0x40, 0xa4, 0xd0, 0xd2, 0x07, 0xe3, 0xd2, 0x11,
    0x97, 0xf0, 0x00, 0xa0, 0xc9, 0x5e, 0xa8, 0x50,
};

std::uint16_t read_u16(const std::span<const std::uint8_t> bytes,
                       const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::int16_t read_i16(const std::span<const std::uint8_t> bytes,
                      const std::size_t offset) noexcept {
    return std::bit_cast<std::int16_t>(read_u16(bytes, offset));
}

template <std::size_t N>
bool matches_utf16le_ascii(const std::span<const std::uint8_t> bytes,
                           const char (&text)[N]) noexcept {
    if (bytes.size() != N * 2) {
        return false;
    }
    for (std::size_t index = 0; index < N; ++index) {
        if (bytes[index * 2] != static_cast<std::uint8_t>(text[index]) ||
            bytes[index * 2 + 1] != 0) {
            return false;
        }
    }
    return true;
}

void copy_capped(const std::span<const std::uint8_t> source,
                 const std::size_t source_offset,
                 const std::size_t payload_length,
                 const std::size_t slot,
                 const std::span<std::uint8_t> output,
                 const bool relative_terminator) noexcept {
    const auto copy_length = std::min(payload_length, kModule33RecordSlotBytes);
    if (copy_length != 0) {
        std::memmove(output.data() + slot, source.data() + source_offset, copy_length);
    }
    // Firmware rounds the capped length down to a halfword and writes a zero
    // halfword two bytes before that boundary. Avoid the firmware's underflow
    // write when a malformed input supplies fewer than two payload bytes.
    if (relative_terminator && copy_length >= 2) {
        const auto terminator = (copy_length & ~std::size_t{1}) - 2;
        output[slot + terminator] = 0;
        output[slot + terminator + 1] = 0;
    }
}

std::size_t destination_slot(const Module33MetadataTag tag,
                             const std::int16_t data_type) noexcept {
    if (data_type == 0) {
        switch (tag) {
        case Module33MetadataTag::album_title: return 4;
        case Module33MetadataTag::genre: return 0;
        case Module33MetadataTag::title: return 3;
        case Module33MetadataTag::track:
        case Module33MetadataTag::track_number: return 1;
        case Module33MetadataTag::title_sort_order: return 3;
        case Module33MetadataTag::album_sort_order: return 4;
        case Module33MetadataTag::artist_sort_order:
        case Module33MetadataTag::author: return 5;
        case Module33MetadataTag::part_of_set: return 2;
        case Module33MetadataTag::unknown: return 6;
        }
    }
    if (data_type == 3) {
        if (tag == Module33MetadataTag::track ||
            tag == Module33MetadataTag::track_number) {
            return 1;
        }
        if (tag == Module33MetadataTag::part_of_set) {
            return 2;
        }
    }
    return 6;
}

} // namespace

Module33AsfHeaderResult decode_module33_asf_header(
    const std::span<const std::uint8_t> header_object,
    const std::span<std::uint8_t> output_area) noexcept {
    Module33AsfHeaderResult result{};
    if (output_area.size() < kModule33RecordOutputBytes) {
        return result;
    }
    std::fill_n(output_area.begin(), kModule33RecordOutputBytes, 0);
    if (header_object.size() < 30 ||
        !std::equal(header_object.begin(), header_object.begin() + 16,
                    kAsfHeaderObjectGuid.begin())) {
        return result;
    }

    result.header_size = read_u16(header_object, 16) |
                         (static_cast<std::uint32_t>(read_u16(header_object, 18)) << 16);
    result.declared_child_count = static_cast<std::uint32_t>(read_u16(header_object, 24)) |
                                  (static_cast<std::uint32_t>(read_u16(header_object, 26)) << 16);
    if (result.header_size < 30 || result.header_size > header_object.size()) {
        return result;
    }

    std::size_t cursor = 30;
    while (cursor < result.header_size && result.metadata_objects_decoded < 2) {
        const auto remaining = static_cast<std::size_t>(result.header_size) - cursor;
        if (remaining < 24) {
            return {};
        }
        const auto child = header_object.subspan(cursor, remaining);
        const auto child_size = static_cast<std::uint32_t>(read_u16(child, 16)) |
                                (static_cast<std::uint32_t>(read_u16(child, 18)) << 16);
        if (child_size < 24 || child_size > remaining) {
            return {};
        }

        const auto child_object = child.first(child_size);
        const auto guid = child_object.first<16>();
        const bool is_metadata =
            std::equal(guid.begin(), guid.end(), kContentDescriptionGuid.begin()) ||
            std::equal(guid.begin(), guid.end(), kExtendedContentDescriptionGuid.begin());
        if (is_metadata) {
            const auto decoded = decode_module33_asf_metadata_object(child_object, output_area);
            if (!decoded.record.valid) {
                return {};
            }
            ++result.metadata_objects_decoded;
        }
        cursor += child_size;
        ++result.child_objects_seen;
    }

    result.valid = true;
    result.bytes_consumed = cursor;
    return result;
}

Module33AsfMetadataObjectResult decode_module33_asf_metadata_object(
    const std::span<const std::uint8_t> object,
    const std::span<std::uint8_t> output_area) noexcept {
    Module33AsfMetadataObjectResult result{};
    if (object.size() < 24) {
        return result;
    }

    const auto guid = object.first<16>();
    if (std::equal(guid.begin(), guid.end(), kContentDescriptionGuid.begin())) {
        result.kind = Module33AsfMetadataObjectKind::content_description;
    } else if (std::equal(guid.begin(), guid.end(),
                          kExtendedContentDescriptionGuid.begin())) {
        result.kind = Module33AsfMetadataObjectKind::extended_content_description;
    } else {
        return result;
    }

    result.object_size = read_u16(object, 16) |
                         (static_cast<std::uint32_t>(read_u16(object, 18)) << 16);
    if (result.object_size < 24 || result.object_size > object.size()) {
        return result;
    }
    const auto body = object.subspan(24, result.object_size - 24);
    if (result.kind == Module33AsfMetadataObjectKind::content_description) {
        result.record = decode_module33_pair_record(body, output_area);
    } else {
        result.record = decode_module33_tagged_record(body, output_area, nullptr);
    }
    return result;
}

Module33MetadataTag classify_module33_wma_metadata_tag(
    const std::span<const std::uint8_t> tag_bytes) noexcept {
    if (matches_utf16le_ascii(tag_bytes, "WM/AlbumTitle")) {
        return Module33MetadataTag::album_title;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/Genre")) {
        return Module33MetadataTag::genre;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/Title")) {
        return Module33MetadataTag::title;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/Track")) {
        return Module33MetadataTag::track;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/TrackNumber")) {
        return Module33MetadataTag::track_number;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/TitleSortOrder")) {
        return Module33MetadataTag::title_sort_order;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/AlbumSortOrder")) {
        return Module33MetadataTag::album_sort_order;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/ArtistSortOrder")) {
        return Module33MetadataTag::artist_sort_order;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/Author")) {
        return Module33MetadataTag::author;
    }
    if (matches_utf16le_ascii(tag_bytes, "WM/PartOfSet")) {
        return Module33MetadataTag::part_of_set;
    }
    return Module33MetadataTag::unknown;
}

Module33RecordParseResult decode_module33_pair_record(
    const std::span<const std::uint8_t> record_body,
    const std::span<std::uint8_t> output_area) noexcept {
    Module33RecordParseResult result{};
    if (output_area.size() < kModule33RecordOutputBytes || record_body.size() < 10) {
        return result;
    }

    const auto first_length = read_i16(record_body, 0);
    const auto second_length = read_i16(record_body, 2);
    if (first_length < 0 || second_length < 0) {
        return result;
    }
    const auto first_size = static_cast<std::size_t>(first_length);
    const auto second_size = static_cast<std::size_t>(second_length);
    if (first_size > record_body.size() - 10 ||
        second_size > record_body.size() - 10 - first_size) {
        return result;
    }

    const auto first_offset = std::size_t{10};
    const auto second_offset = first_offset + first_size;
    copy_capped(record_body, first_offset, first_size, 0x180, output_area, false);
    copy_capped(record_body, second_offset, second_size, 0x280, output_area, false);
    output_area[0x1fe] = 0;
    output_area[0x1ff] = 0;
    output_area[0x2fe] = 0;
    output_area[0x2ff] = 0;

    result.valid = true;
    result.bytes_consumed = 10 + first_size + second_size;
    result.entries_processed = 2;
    result.payloads_copied = 2;
    return result;
}

Module33RecordParseResult decode_module33_tagged_record(
    const std::span<const std::uint8_t> record_body,
    const std::span<std::uint8_t> output_area,
    const Module33MetadataTagMatcher matcher) noexcept {
    Module33RecordParseResult result{};
    if (output_area.size() < kModule33RecordOutputBytes || record_body.size() < 2) {
        return result;
    }

    const auto entry_count = read_i16(record_body, 0);
    std::size_t cursor = 2;
    result.valid = true;
    if (entry_count <= 0) {
        result.bytes_consumed = cursor;
        return result;
    }

    while (result.entries_processed < static_cast<std::size_t>(entry_count) &&
           result.payloads_copied < 9) {
        if (cursor > record_body.size() || record_body.size() - cursor < 2) {
            return {};
        }
        const auto tag_length = read_i16(record_body, cursor);
        cursor += 2;
        if (tag_length < 0) {
            return {};
        }
        const auto label_size = static_cast<std::size_t>(tag_length);
        if (label_size > record_body.size() - cursor ||
            record_body.size() - cursor - label_size < 4) {
            return {};
        }

        const auto tag_bytes = record_body.subspan(cursor, label_size);
        const auto tag = matcher == nullptr ? classify_module33_wma_metadata_tag(tag_bytes)
                                            : matcher(tag_bytes);
        cursor += label_size;
        const auto data_type = read_i16(record_body, cursor);
        const auto payload_length = static_cast<std::size_t>(read_u16(record_body, cursor + 2));
        cursor += 4;
        if (payload_length > record_body.size() - cursor) {
            return {};
        }

        const auto slot = destination_slot(tag, data_type);
        if (slot < 6) {
            copy_capped(record_body, cursor, payload_length,
                        slot * kModule33RecordSlotBytes, output_area, true);
            ++result.payloads_copied;
        }
        cursor += payload_length;
        ++result.entries_processed;
    }

    result.bytes_consumed = cursor;
    return result;
}

} // namespace echo::audio
