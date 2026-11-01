#include "echo/image/bmp_info.hpp"
#include "echo/image/source_pixel.hpp"

#include <bit>
#include <limits>

namespace echo::image {
namespace {

std::uint16_t read_u16(const std::span<const std::uint8_t> bytes,
                       const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset])
        | static_cast<std::uint16_t>(bytes[offset + 1] << 8U);
}

std::uint32_t read_u32(const std::span<const std::uint8_t> bytes,
                       const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

} // namespace

std::optional<BmpInfo> parse_bmp_info(
    const std::span<const std::uint8_t> bytes) noexcept {
    constexpr std::size_t file_header_size = 14;
    constexpr std::size_t info_header_min_size = 40;
    constexpr std::size_t minimum_size = file_header_size + info_header_min_size;
    if (bytes.size() < minimum_size || bytes[0] != 'B' || bytes[1] != 'M') {
        return std::nullopt;
    }

    BmpInfo result{};
    result.file_size = read_u32(bytes, 2);
    result.pixel_offset = read_u32(bytes, 10);
    result.dib_header_size = read_u32(bytes, file_header_size);
    if (result.dib_header_size < info_header_min_size
        || result.dib_header_size > bytes.size() - file_header_size) {
        return std::nullopt;
    }

    const auto header_end = file_header_size + result.dib_header_size;
    if (result.file_size < header_end || result.file_size > bytes.size()
        || result.pixel_offset < header_end || result.pixel_offset > result.file_size) {
        return std::nullopt;
    }

    result.width = std::bit_cast<std::int32_t>(read_u32(bytes, 18));
    result.height = std::bit_cast<std::int32_t>(read_u32(bytes, 22));
    const auto planes = read_u16(bytes, 26);
    result.bits_per_pixel = read_u16(bytes, 28);
    result.compression = read_u32(bytes, 30);
    const auto mode = source_pixel_mode_for(result.bits_per_pixel,
                                            result.compression);
    if (result.width <= 0 || result.height == 0 || planes != 1 || !mode) {
        return std::nullopt;
    }

    if (result.bits_per_pixel <= 8U) {
        result.palette_entry_count = static_cast<std::uint16_t>(
            1U << result.bits_per_pixel);
        result.palette_bytes = static_cast<std::size_t>(
            result.palette_entry_count) * 4U;
    }
    if (result.palette_bytes > result.pixel_offset - header_end) {
        return std::nullopt;
    }

    const auto width = static_cast<std::uint64_t>(result.width);
    const auto height = result.height < 0
        ? static_cast<std::uint64_t>(-static_cast<std::int64_t>(result.height))
        : static_cast<std::uint64_t>(result.height);
    const auto row_bits = width * result.bits_per_pixel;
    const auto row_bytes = ((row_bits + 31U) / 32U) * 4U;
    if (row_bytes > std::numeric_limits<std::uint64_t>::max() / height) {
        return std::nullopt;
    }
    const auto image_bytes = row_bytes * height;
    const auto available = static_cast<std::uint64_t>(result.file_size - result.pixel_offset);
    if (image_bytes > available || image_bytes > std::numeric_limits<std::size_t>::max()) {
        return std::nullopt;
    }

    result.pixel_data_bytes = static_cast<std::size_t>(image_bytes);
    result.top_down = result.height < 0;
    return result;
}

} // namespace echo::image
