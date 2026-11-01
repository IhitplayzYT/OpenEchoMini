#include "echo/resources/embedded_image.hpp"

namespace echo::resources {

std::optional<EmbeddedImageSignature> find_embedded_image(
    std::span<const std::uint8_t> bytes) noexcept {
    for (std::size_t offset = 0; offset + 1 < bytes.size(); ++offset) {
        if (bytes[offset] == 0xff && bytes[offset + 1] == 0xd8) {
            return EmbeddedImageSignature{EmbeddedImageKind::jpeg, offset};
        }
        if (bytes[offset] == 0x42 && bytes[offset + 1] == 0x4d) {
            return EmbeddedImageSignature{EmbeddedImageKind::bmp, offset};
        }
    }
    return std::nullopt;
}

} // namespace echo::resources
