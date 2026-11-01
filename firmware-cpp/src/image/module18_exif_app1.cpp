#include "echo/image/module18_exif_app1.hpp"

#include <utility>

namespace echo::image {
namespace {

std::uint16_t read_be16(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[offset]) << 8U)
        | bytes[offset + 1U]);
}

std::uint16_t read_tiff16(const std::span<const std::uint8_t> bytes,
                          const std::size_t offset,
                          const TiffByteOrder order) noexcept {
    if (order == TiffByteOrder::big_endian) return read_be16(bytes, offset);
    return static_cast<std::uint16_t>(bytes[offset]
        | (static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U));
}

std::uint32_t read_u32(const std::span<const std::uint8_t> bytes,
                       const std::size_t offset,
                       const TiffByteOrder order) noexcept {
    if (order == TiffByteOrder::big_endian) {
        return (static_cast<std::uint32_t>(bytes[offset]) << 24U)
            | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U)
            | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U)
            | static_cast<std::uint32_t>(bytes[offset + 3U]);
    }
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

} // namespace

std::optional<Module18ExifApp1> module18_find_exif_app1(
    const std::span<const std::uint8_t> jpeg) noexcept {
    if (jpeg.size() < 4U || jpeg[0] != 0xffU || jpeg[1] != 0xd8U) {
        return std::nullopt;
    }

    std::size_t cursor = 2U;
    while (cursor < jpeg.size()) {
        if (jpeg[cursor] != 0xffU) return std::nullopt;
        const auto marker_start = cursor;
        while (cursor < jpeg.size() && jpeg[cursor] == 0xffU) ++cursor;
        if (cursor >= jpeg.size()) return std::nullopt;
        const auto marker = jpeg[cursor++];
        if (marker == 0xdbU || marker < 0xe0U || marker > 0xefU) {
            return std::nullopt;
        }
        if (cursor + 2U > jpeg.size()) return std::nullopt;
        const auto segment_length = read_be16(jpeg, cursor);
        if (segment_length < 2U || segment_length > jpeg.size() - cursor) {
            return std::nullopt;
        }
        const auto payload = cursor + 2U;
        const auto segment_end = cursor + segment_length;
        if (marker == 0xe1U && segment_length >= 16U
            && jpeg[payload] == 'E' && jpeg[payload + 1U] == 'x'
            && jpeg[payload + 2U] == 'i' && jpeg[payload + 3U] == 'f'
            && jpeg[payload + 4U] == 0U && jpeg[payload + 5U] == 0U) {
            const auto tiff = payload + 6U;
            TiffByteOrder order;
            if (jpeg[tiff] == 'I' && jpeg[tiff + 1U] == 'I') {
                order = TiffByteOrder::little_endian;
            } else if (jpeg[tiff] == 'M' && jpeg[tiff + 1U] == 'M') {
                order = TiffByteOrder::big_endian;
            } else {
                return std::nullopt;
            }
            const auto magic = order == TiffByteOrder::big_endian
                ? read_be16(jpeg, tiff + 2U)
                : static_cast<std::uint16_t>(jpeg[tiff + 2U]
                    | (static_cast<std::uint16_t>(jpeg[tiff + 3U]) << 8U));
            if (magic != 42U) return std::nullopt;
            const auto ifd_offset = read_u32(jpeg, tiff + 4U, order);
            const auto tiff_bytes = segment_end - tiff;
            if (ifd_offset > tiff_bytes || tiff_bytes - ifd_offset < 2U) {
                return std::nullopt;
            }
            return Module18ExifApp1{marker_start, segment_length, order, ifd_offset};
        }
        cursor = segment_end;
    }
    return std::nullopt;
}

std::optional<Module18ExifThumbnailLocation> module18_find_exif_thumbnail(
    const std::span<const std::uint8_t> jpeg) noexcept {
    const auto app1 = module18_find_exif_app1(jpeg);
    if (!app1) return std::nullopt;
    const auto tiff = app1->marker_offset + 10U;
    const auto segment_end = app1->marker_offset + 2U + app1->segment_length;
    if (segment_end > jpeg.size() || tiff > segment_end) return std::nullopt;
    const auto tiff_size = segment_end - tiff;

    const auto get_directory_next = [&](const std::uint32_t directory_offset)
        -> std::optional<std::pair<std::size_t, std::uint16_t>> {
        if (directory_offset > tiff_size || tiff_size - directory_offset < 2U) {
            return std::nullopt;
        }
        const auto directory = tiff + directory_offset;
        const auto count = read_tiff16(jpeg, directory, app1->byte_order);
        const auto entries_start = directory + 2U;
        const auto entries_remaining = segment_end - entries_start;
        if (entries_remaining < 4U
            || count > (entries_remaining - 4U) / 12U) {
            return std::nullopt;
        }
        return std::pair{entries_start, count};
    };

    const auto ifd0 = get_directory_next(app1->first_ifd_offset);
    if (!ifd0) return std::nullopt;
    const auto next_ifd_offset = read_u32(
        jpeg, ifd0->first + static_cast<std::size_t>(ifd0->second) * 12U,
        app1->byte_order);
    if (next_ifd_offset == 0U) return std::nullopt;

    const auto ifd1 = get_directory_next(next_ifd_offset);
    if (!ifd1) return std::nullopt;
    std::optional<std::uint32_t> jpeg_offset;
    std::optional<std::uint32_t> jpeg_length;
    for (std::uint16_t i = 0; i < ifd1->second; ++i) {
        const auto entry = ifd1->first + static_cast<std::size_t>(i) * 12U;
        const auto tag = read_tiff16(jpeg, entry, app1->byte_order);
        if (tag != 0x0201U && tag != 0x0202U) continue;
        const auto type = read_tiff16(jpeg, entry + 2U, app1->byte_order);
        const auto count = read_u32(jpeg, entry + 4U, app1->byte_order);
        if (count != 1U || (type != 3U && type != 4U)) return std::nullopt;
        const auto value = type == 3U
            ? static_cast<std::uint32_t>(read_tiff16(jpeg, entry + 8U, app1->byte_order))
            : read_u32(jpeg, entry + 8U, app1->byte_order);
        if (tag == 0x0201U) jpeg_offset = value;
        else jpeg_length = value;
    }
    if (!jpeg_offset || !jpeg_length || *jpeg_offset > tiff_size
        || *jpeg_length > tiff_size - *jpeg_offset) {
        return std::nullopt;
    }
    return Module18ExifThumbnailLocation{tiff + *jpeg_offset, *jpeg_length};
}

} // namespace echo::image
