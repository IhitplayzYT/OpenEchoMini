#pragma once

#include <cstdint>
#include <optional>

namespace echo::audio {

struct Module18CoverArtLayout {
    std::uint16_t requested_width{};
    std::uint16_t requested_height{};
    std::uint16_t fitted_width{};
    std::uint16_t fitted_height{};
    std::uint16_t vertical_offset{};
};

struct Module18CoverArtRow {
    std::uint16_t x{};
    std::uint16_t y{};
    std::uint16_t width{};
    std::uint16_t height{1};
    std::uint8_t blit_mode{0x11};
};

struct Module18CoverArtOrigin {
    std::uint16_t x{};
    std::uint16_t y{};
};

// Applies the firmware's ratio-based margins within its 126-pixel display box.
[[nodiscard]] std::optional<Module18CoverArtOrigin>
make_module18_cover_art_origin(std::uint32_t x_origin,
                               std::uint32_t y_origin,
                               std::int32_t dimension_a,
                               std::int32_t dimension_b) noexcept;

// The firmware centers fitted rows vertically when the render-state flag is 0.
[[nodiscard]] std::optional<Module18CoverArtLayout> make_module18_cover_art_layout(
    std::uint32_t requested_width,
    std::uint32_t requested_height,
    std::uint32_t fitted_width,
    std::uint32_t fitted_height,
    bool center_vertically) noexcept;

// row_index is relative to the fitted image. The JPEG decoder's true
// continuation result selects the observed width-minus-one branch.
[[nodiscard]] std::optional<Module18CoverArtRow> make_module18_cover_art_row(
    const Module18CoverArtLayout& layout,
    std::uint32_t x_origin,
    std::uint32_t y_origin,
    std::uint32_t row_index,
    bool jpeg_rows_remain) noexcept;

} // namespace echo::audio
