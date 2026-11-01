#pragma once

#include <cstdint>
#include <optional>

namespace echo::image {

// Module 20 advances source coordinates in 16.16 fixed point. This models one
// axis of the observed nearest-sample cursor; framebuffer clipping and row
// callback behavior remain separate.
struct Module20ScaleAxis {
    std::uint32_t source_extent{};
    std::uint32_t destination_extent{};
    std::uint32_t step_16_16{};

    [[nodiscard]] std::optional<std::uint32_t> source_coordinate(
        std::uint32_t destination_coordinate) const noexcept;
};

struct Module20FitSize {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t horizontal_offset{};
    bool native_size{};
};

// Models the no-enlargement branch, ratio fit, and centered output row.
[[nodiscard]] std::optional<Module20FitSize> make_module20_fit_size(
    std::uint32_t source_width,
    std::uint32_t source_height,
    std::uint32_t requested_width,
    std::uint32_t requested_height) noexcept;

[[nodiscard]] std::optional<Module20ScaleAxis> make_module20_scale_axis(
    std::uint32_t source_extent,
    std::uint32_t destination_extent) noexcept;

} // namespace echo::image
