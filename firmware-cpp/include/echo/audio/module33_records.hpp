#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::audio {

inline constexpr std::size_t kModule33RecordOutputBytes = 0x300;
inline constexpr std::size_t kModule33RecordSlotBytes = 0x80;

struct Module33RecordParseResult {
    bool valid{};
    std::size_t bytes_consumed{};
    std::size_t entries_processed{};
    std::size_t payloads_copied{};
};

enum class Module33AsfMetadataObjectKind : std::uint8_t {
    unknown,
    content_description,
    extended_content_description,
};

struct Module33AsfMetadataObjectResult {
    Module33AsfMetadataObjectKind kind{Module33AsfMetadataObjectKind::unknown};
    std::uint32_t object_size{};
    Module33RecordParseResult record{};
};

struct Module33AsfHeaderResult {
    bool valid{};
    std::uint32_t header_size{};
    std::uint32_t declared_child_count{};
    std::size_t child_objects_seen{};
    std::size_t metadata_objects_decoded{};
    std::size_t bytes_consumed{};
};

// Walks one complete ASF Header Object as FUN_0308b840 does. The first 0x300
// output bytes are cleared before parsing, the child scan starts after the
// 30-byte header, unknown GUIDs are skipped by size, and at most two metadata
// objects are decoded.
[[nodiscard]] Module33AsfHeaderResult decode_module33_asf_header(
    std::span<const std::uint8_t> header_object,
    std::span<std::uint8_t> output_area) noexcept;

// Dispatches the two ASF metadata objects compared by FUN_0308b840. Input is
// one complete object (16-byte GUID, 8-byte size, then body); only the low
// 32-bit size field used by the firmware is interpreted.
[[nodiscard]] Module33AsfMetadataObjectResult decode_module33_asf_metadata_object(
    std::span<const std::uint8_t> object,
    std::span<std::uint8_t> output_area) noexcept;

// Parses the record shape handled by FUN_0308b760. `record_body` begins after
// the outer 24-byte record header. The two signed lengths precede six reserved
// bytes and two payload blocks. Each payload copy is capped at 0x80 bytes.
[[nodiscard]] Module33RecordParseResult decode_module33_pair_record(
    std::span<const std::uint8_t> record_body,
    std::span<std::uint8_t> output_area) noexcept;

enum class Module33MetadataTag : std::uint8_t {
    unknown,
    album_title,
    genre,
    title,
    track,
    track_number,
    title_sort_order,
    album_sort_order,
    artist_sort_order,
    author,
    part_of_set,
};

using Module33MetadataTagMatcher = Module33MetadataTag (*)(
    std::span<const std::uint8_t> tag_bytes) noexcept;

// Matches the exact UTF-16LE, NUL-terminated WMA field names recovered from
// module 33's data section. Unknown keys return `unknown`.
[[nodiscard]] Module33MetadataTag classify_module33_wma_metadata_tag(
    std::span<const std::uint8_t> tag_bytes) noexcept;

// Parses the counted tag/type/length/payload records handled by FUN_0308b37a.
// `matcher` may override the built-in WMA field classifier for experiments
// with another table. Passing null uses the signatures observed in the image.
[[nodiscard]] Module33RecordParseResult decode_module33_tagged_record(
    std::span<const std::uint8_t> record_body,
    std::span<std::uint8_t> output_area,
    Module33MetadataTagMatcher matcher) noexcept;

} // namespace echo::audio
