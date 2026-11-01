#include "echo/image/jpeg_frame_descriptor.hpp"

#include "echo/bitstream/msb_bit_reader.hpp"

namespace echo::image {

std::optional<JpegFrameDescriptor> parse_jpeg_frame_descriptor(
    const std::span<const std::uint8_t> bytes) noexcept {
    bitstream::MsbBitReader reader(bytes);
    const auto declared_length = reader.read(16);
    const auto marker = reader.read(8);
    const auto width = reader.read(16);
    const auto height = reader.read(16);
    const auto component_count = reader.read(8);
    if (!declared_length || !marker || !width || !height || !component_count
        || *marker != 8 || *width == 0 || *width > 8000
        || *height == 0 || *height > 8000 || *component_count > 3
        || *declared_length != 8 + 3 * *component_count) {
        return std::nullopt;
    }

    JpegFrameDescriptor result{
        static_cast<std::uint16_t>(*declared_length),
        static_cast<std::uint16_t>(*width),
        static_cast<std::uint16_t>(*height),
        static_cast<std::uint8_t>(*component_count),
        {},
    };
    for (std::size_t i = 0; i < result.component_count; ++i) {
        const auto id = reader.read(8);
        const auto field_a = reader.read(4);
        const auto field_b = reader.read(4);
        const auto selector = reader.read(8);
        if (!id || !field_a || !field_b || !selector) {
            return std::nullopt;
        }
        result.components[i] = JpegComponentFields{
            static_cast<std::uint8_t>(*id),
            static_cast<std::uint8_t>(*field_a),
            static_cast<std::uint8_t>(*field_b),
            static_cast<std::uint8_t>(*selector),
        };
    }
    return result;
}

} // namespace echo::image
