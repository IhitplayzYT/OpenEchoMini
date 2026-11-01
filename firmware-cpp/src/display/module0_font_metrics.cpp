#include "echo/display/module0_font_metrics.hpp"

namespace echo::display {

std::uint16_t module0_glyph_advance(Module0GlyphMetricInput input) noexcept {
    const std::uint16_t default_width = input.small_font ? 12U : 16U;
    std::uint16_t width = input.small_font ? 6U : 8U;
    const auto metadata = input.metadata_high_byte;

    if (metadata == 0x10U) {
        if (input.display_mode == 0x0aU) width = 8U;
    } else if (metadata != 0x20U) {
        if (metadata > 0x80U) {
            width = static_cast<std::uint16_t>(metadata - 0x80U);
        } else if (metadata == 0x80U && input.codepoint >= 0x7fU) {
            width = default_width;
        }
    }

    if (input.codepoint >= 0x1100U && input.codepoint < 0x1200U) {
        width = default_width;
    }
    if (input.display_mode == 0x0cU && input.special_character_class) {
        width = 0;
    }
    if (input.codepoint >= 0xe468U && input.codepoint < 0xe4a4U) {
        width = static_cast<std::uint16_t>(width + 1U);
    }
    return width;
}

} // namespace echo::display
