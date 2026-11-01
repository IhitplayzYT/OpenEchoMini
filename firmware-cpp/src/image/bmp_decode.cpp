#include "echo/image/bmp_decode.hpp"

#include <limits>

namespace echo::image {

std::optional<DecodedBmp> decode_bmp(
    const std::span<const std::uint8_t> bytes) {
    const auto info = parse_bmp_info(bytes);
    if (!info) return std::nullopt;

    const auto width = static_cast<std::size_t>(info->width);
    const auto signed_height = static_cast<std::int64_t>(info->height);
    const auto height = static_cast<std::size_t>(
        signed_height < 0 ? -signed_height : signed_height);
    if (height == 0 || width > std::numeric_limits<std::size_t>::max() / height) {
        return std::nullopt;
    }

    DecodedBmp result{};
    result.width = static_cast<std::uint32_t>(width);
    result.height = static_cast<std::uint32_t>(height);
    const auto pixel_count = width * height;
    if (pixel_count > result.pixels.max_size()) return std::nullopt;

    const auto header_end = 14U + static_cast<std::size_t>(info->dib_header_size);
    if (header_end > bytes.size() || info->palette_bytes > bytes.size() - header_end) {
        return std::nullopt;
    }
    const auto palette = bytes.subspan(header_end, info->palette_bytes);

    const auto row_bits = static_cast<std::uint64_t>(width) * info->bits_per_pixel;
    const auto row_stride_64 = ((row_bits + 31U) / 32U) * 4U;
    if (row_stride_64 > std::numeric_limits<std::size_t>::max()) {
        return std::nullopt;
    }
    const auto row_stride = static_cast<std::size_t>(row_stride_64);
    if (row_stride > info->pixel_data_bytes / height) return std::nullopt;

    const auto mode = source_pixel_mode_for(info->bits_per_pixel,
                                            info->compression);
    if (!mode) return std::nullopt;
    result.pixels.reserve(pixel_count);
    for (std::size_t y = 0; y < height; ++y) {
        const auto file_y = info->top_down ? y : height - 1U - y;
        const auto row_offset = static_cast<std::size_t>(info->pixel_offset)
            + file_y * row_stride;
        if (row_offset > bytes.size() || row_stride > bytes.size() - row_offset) {
            return std::nullopt;
        }
        const auto row = bytes.subspan(row_offset, row_stride);
        for (std::size_t x = 0; x < width; ++x) {
            const auto pixel = decode_source_pixel(*mode, row, x, palette);
            if (!pixel) return std::nullopt;
            result.pixels.push_back(*pixel);
        }
    }
    return result;
}

} // namespace echo::image
