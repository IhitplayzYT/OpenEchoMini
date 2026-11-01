#include "echo/audio/module18_cover_art_layout.hpp"

#include <limits>

namespace echo::audio {

std::optional<Module18CoverArtOrigin> make_module18_cover_art_origin(
    const std::uint32_t x_origin,
    const std::uint32_t y_origin,
    const std::int32_t dimension_a,
    const std::int32_t dimension_b) noexcept {
    constexpr std::int32_t display_extent = 126;
    if (x_origin > std::numeric_limits<std::uint16_t>::max()
        || y_origin > std::numeric_limits<std::uint16_t>::max()
        || dimension_a <= 0 || dimension_b <= 0
        || dimension_a > 0x7fff || dimension_b > 0x7fff) {
        return std::nullopt;
    }
    auto adjusted_x = x_origin;
    auto adjusted_y = y_origin;
    if (dimension_b < dimension_a) {
        const auto margin = (display_extent - dimension_b * display_extent / dimension_a) / 2;
        adjusted_x += static_cast<std::uint32_t>(margin);
    } else if (dimension_a < dimension_b) {
        const auto margin = (display_extent - dimension_a * display_extent / dimension_b) / 2;
        adjusted_y += static_cast<std::uint32_t>(margin);
    }
    if (adjusted_x > std::numeric_limits<std::uint16_t>::max()
        || adjusted_y > std::numeric_limits<std::uint16_t>::max()) {
        return std::nullopt;
    }
    return Module18CoverArtOrigin{
        static_cast<std::uint16_t>(adjusted_x),
        static_cast<std::uint16_t>(adjusted_y),
    };
}

std::optional<Module18CoverArtLayout> make_module18_cover_art_layout(
    const std::uint32_t requested_width,
    const std::uint32_t requested_height,
    const std::uint32_t fitted_width,
    const std::uint32_t fitted_height,
    const bool center_vertically) noexcept {
    constexpr std::uint32_t max_dimension = 0xffffU;
    if (requested_width == 0 || requested_height == 0
        || fitted_width == 0 || fitted_height == 0
        || requested_width > max_dimension || requested_height > max_dimension
        || fitted_width > requested_width || fitted_height > requested_height) {
        return std::nullopt;
    }
    const auto vertical_offset = center_vertically
        ? (requested_height - fitted_height) / 2U : 0U;
    return Module18CoverArtLayout{
        static_cast<std::uint16_t>(requested_width),
        static_cast<std::uint16_t>(requested_height),
        static_cast<std::uint16_t>(fitted_width),
        static_cast<std::uint16_t>(fitted_height),
        static_cast<std::uint16_t>(vertical_offset),
    };
}

std::optional<Module18CoverArtRow> make_module18_cover_art_row(
    const Module18CoverArtLayout& layout,
    const std::uint32_t x_origin,
    const std::uint32_t y_origin,
    const std::uint32_t row_index,
    const bool jpeg_rows_remain) noexcept {
    if (layout.requested_width == 0 || layout.fitted_height == 0
        || layout.fitted_width == 0 || layout.fitted_width > layout.requested_width
        || layout.vertical_offset + layout.fitted_height > layout.requested_height
        || row_index >= layout.fitted_height
        || (jpeg_rows_remain && layout.requested_width == 1U)) {
        return std::nullopt;
    }

    const auto y = static_cast<std::uint64_t>(y_origin)
        + layout.vertical_offset + row_index;
    if (x_origin > std::numeric_limits<std::uint16_t>::max()
        || y > std::numeric_limits<std::uint16_t>::max()) {
        return std::nullopt;
    }
    const auto width = static_cast<std::uint16_t>(layout.requested_width
        - (jpeg_rows_remain ? 1U : 0U));
    return Module18CoverArtRow{
        static_cast<std::uint16_t>(x_origin),
        static_cast<std::uint16_t>(y),
        width,
        1,
        0x11,
    };
}

} // namespace echo::audio
