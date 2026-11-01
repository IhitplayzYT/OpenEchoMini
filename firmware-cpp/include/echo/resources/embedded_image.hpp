#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::resources {

enum class EmbeddedImageKind {
    jpeg,
    bmp,
};

struct EmbeddedImageSignature {
    EmbeddedImageKind kind{};
    std::size_t offset{};
};

// Finds the first JPEG SOI or BMP file signature in a bounded metadata buffer.
// This mirrors the firmware's signature search; it does not validate or decode
// the image payload.
[[nodiscard]] std::optional<EmbeddedImageSignature> find_embedded_image(
    std::span<const std::uint8_t> bytes) noexcept;

} // namespace echo::resources
