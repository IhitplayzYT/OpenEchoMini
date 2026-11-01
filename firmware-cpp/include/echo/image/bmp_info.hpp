#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::image {

struct BmpInfo {
    std::uint32_t file_size{};
    std::uint32_t pixel_offset{};
    std::uint32_t dib_header_size{};
    std::int32_t width{};
    std::int32_t height{};
    std::uint16_t bits_per_pixel{};
    std::uint32_t compression{};
    std::uint16_t palette_entry_count{};
    std::size_t palette_bytes{};
    std::size_t pixel_data_bytes{};
    bool top_down{};
};

// Parses the BMP combinations routed to observed module 20 pixel kernels:
// compression 0 at 1/4/8/16/24/32 bpp, and compression 3 at 16/32 bpp.
// Indexed modes require their full 2^bpp four-byte palette. For compression 3,
// bitfield masks are retained in the pre-pixel area but not interpreted, as
// the firmware kernels use fixed 565/channel-byte layouts.
[[nodiscard]] std::optional<BmpInfo> parse_bmp_info(
    std::span<const std::uint8_t> bytes) noexcept;

} // namespace echo::image
