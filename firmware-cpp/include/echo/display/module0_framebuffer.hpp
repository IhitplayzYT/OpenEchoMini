#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::display {

enum class Module0FramebufferRegion : std::uint8_t {
    upper_rows,
    lower_rows,
};

struct Module0FramebufferPixel {
    Module0FramebufferRegion region{};
    std::size_t byte_offset{};
};

enum class Module0ImageBlitMode : std::uint8_t {
    rgb565 = 0x10,
    clipped_rgb565 = 0x11,
};

// Maps coordinates to module 0's split 16-bit framebuffer storage.
[[nodiscard]] std::optional<Module0FramebufferPixel>
module0_framebuffer_pixel(std::uint16_t x, std::uint16_t y,
                          std::uint16_t bank) noexcept;

// Writes a little-endian pixel into the selected caller-provided region.
// Dirty-rectangle updates and draw-mode color substitution are separate paths.
[[nodiscard]] bool write_module0_framebuffer_pixel(
    std::span<std::uint8_t> upper_rows,
    std::span<std::uint8_t> lower_rows,
    std::uint16_t x, std::uint16_t y, std::uint16_t bank,
    std::uint16_t color) noexcept;

// Reconstructs module 0's 16-bit rectangle path used by the embedded-image
// renderer. Mode 0x11 clips right/bottom to x=156/y=146. Set
// replace_key_color when the observed display draw state equals 2.
[[nodiscard]] bool write_module0_image_rect(
    std::span<std::uint8_t> upper_rows,
    std::span<std::uint8_t> lower_rows,
    std::span<const std::uint16_t> source_pixels,
    std::uint16_t x, std::uint16_t y,
    std::uint16_t width, std::uint16_t height,
    std::uint16_t bank,
    Module0ImageBlitMode mode,
    bool replace_key_color = false) noexcept;

// Models module 0's mode-1 one-word-per-row monochrome mask path. Flag bit 0
// preserves zero bits; flag bit 1 swaps the set/clear color selection.
[[nodiscard]] bool write_module0_mono_mask(
    std::span<std::uint8_t> upper_rows,
    std::span<std::uint8_t> lower_rows,
    std::span<const std::uint16_t> row_masks,
    std::uint16_t x, std::uint16_t y,
    std::uint16_t width, std::uint16_t height,
    std::uint16_t bank,
    std::uint16_t set_color, std::uint16_t clear_color,
    std::uint8_t draw_flags) noexcept;

} // namespace echo::display
