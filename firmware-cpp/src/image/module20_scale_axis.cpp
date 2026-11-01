#include "echo/image/module20_scale_axis.hpp"

#include <limits>

namespace echo::image {

std::optional<Module20FitSize> make_module20_fit_size(
    const std::uint32_t source_width,
    const std::uint32_t source_height,
    const std::uint32_t requested_width,
    const std::uint32_t requested_height) noexcept {
    if (source_width == 0 || source_height == 0
        || source_width > 0xffffU || source_height > 0xffffU
        || requested_width == 0 || requested_height == 0) {
        return std::nullopt;
    }

    if (source_width < requested_width && source_height < requested_height) {
        return Module20FitSize{
            source_width, source_height, (requested_width - source_width) / 2U, true};
    }

    const auto width_scale = static_cast<float>(requested_width)
        / static_cast<float>(source_width);
    const auto height_scale = static_cast<float>(requested_height)
        / static_cast<float>(source_height);
    const auto scale = width_scale < height_scale ? width_scale : height_scale;
    std::uint32_t width = static_cast<std::uint32_t>(
        static_cast<double>(source_width) * static_cast<double>(scale));
    std::uint32_t height = static_cast<std::uint32_t>(
        static_cast<double>(source_height) * static_cast<double>(scale));
    if (width == 0) width = 1;
    if (height == 0) height = 1;
    return Module20FitSize{width, height, (requested_width - width) / 2U, false};
}

std::optional<std::uint32_t> Module20ScaleAxis::source_coordinate(
    std::uint32_t destination_coordinate) const noexcept {
    if (destination_extent == 0 || destination_coordinate >= destination_extent) {
        return std::nullopt;
    }
    const auto cursor = static_cast<std::uint64_t>(destination_coordinate)
        * step_16_16;
    const auto coordinate = static_cast<std::uint32_t>(cursor >> 16U);
    if (coordinate >= source_extent) return std::nullopt;
    return coordinate;
}

std::optional<Module20ScaleAxis> make_module20_scale_axis(
    std::uint32_t source_extent,
    std::uint32_t destination_extent) noexcept {
    // The observed firmware takes source dimensions through a 16-bit value
    // before constructing the fixed-point numerator.
    if (source_extent == 0 || source_extent > 0xffffU || destination_extent == 0) {
        return std::nullopt;
    }
    const auto step = ((static_cast<std::uint64_t>(source_extent) << 16U)
                       / destination_extent) + 1U;
    if (step > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    return Module20ScaleAxis{
        source_extent,
        destination_extent,
        static_cast<std::uint32_t>(step),
    };
}

} // namespace echo::image
