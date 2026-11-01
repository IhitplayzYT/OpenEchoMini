#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::image {

struct JpegComponentFields {
    std::uint8_t id{};
    std::uint8_t field_a{};
    std::uint8_t field_b{};
    std::uint8_t selector{};
};

struct JpegFrameDescriptor {
    std::uint16_t declared_header_length{};
    std::uint16_t width{};
    std::uint16_t height{};
    std::uint8_t component_count{};
    std::array<JpegComponentFields, 3> components{};
};

// Parses module 19's length-checked frame descriptor. Component fields are
// kept neutral because their sampling/table meanings are not resolved.
[[nodiscard]] std::optional<JpegFrameDescriptor> parse_jpeg_frame_descriptor(
    std::span<const std::uint8_t> bytes) noexcept;

} // namespace echo::image
