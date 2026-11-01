#pragma once

#include <cstdint>

namespace echo::display {

struct Module0GlyphMetricInput {
    std::uint16_t codepoint{};
    std::uint8_t metadata_high_byte{};
    bool small_font{};
    std::uint8_t display_mode{};
    // Result of FUN_03057942 when display_mode is 0x0c.
    bool special_character_class{};
};

// Reproduces the width selection in FUN_030636c8 from the glyph metadata byte
// and display context. Glyph-table I/O and text shaping remain separate.
[[nodiscard]] std::uint16_t module0_glyph_advance(
    Module0GlyphMetricInput input) noexcept;

} // namespace echo::display
