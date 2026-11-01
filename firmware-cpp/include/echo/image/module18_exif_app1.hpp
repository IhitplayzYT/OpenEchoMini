#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::image {

enum class TiffByteOrder {
    little_endian,
    big_endian,
};

struct Module18ExifApp1 {
    std::size_t marker_offset{};
    std::uint16_t segment_length{};
    TiffByteOrder byte_order{};
    std::uint32_t first_ifd_offset{};
};

struct Module18ExifThumbnailLocation {
    std::size_t file_offset{};
    std::uint32_t byte_length{};
};

// Finds the APP1 Exif segment before the first DQT marker and validates its
// TIFF byte-order marker, magic value, and first-IFD offset.
[[nodiscard]] std::optional<Module18ExifApp1> module18_find_exif_app1(
    std::span<const std::uint8_t> jpeg) noexcept;

// Reads TIFF IFD1 tags 0x0201 (JPEG offset) and 0x0202 (JPEG length), requiring
// the complete thumbnail range to remain inside the APP1 segment.
[[nodiscard]] std::optional<Module18ExifThumbnailLocation>
module18_find_exif_thumbnail(std::span<const std::uint8_t> jpeg) noexcept;

} // namespace echo::image
