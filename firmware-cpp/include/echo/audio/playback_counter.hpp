#pragma once

#include <array>
#include <cstdint>

namespace echo::audio {

struct PlaybackCounterGlyph {
    std::uint16_t glyph_id;
    std::uint16_t x;
    std::uint16_t y;
};

struct PlaybackTripleCounter {
    std::array<PlaybackCounterGlyph, 8> glyphs{};
};

struct PlaybackNumericPair {
    std::array<PlaybackCounterGlyph, 9> glyphs{};
    std::uint8_t glyph_count{};
};

// Models module 0's seconds-to-HH:MM:SS formatter and module 27's
// FUN_0307cd80 glyph renderer. Hours wrap to their final two displayed digits.
// alternate_position selects the second observed horizontal origin.
[[nodiscard]] PlaybackTripleCounter format_playback_time_counter(
    std::uint32_t total_seconds,
    bool alternate_position) noexcept;

// Models FUN_0307c460's integer-pair display. The first value is clamped to
// limit; each side is rendered without leading zeroes. At 10000 and above the
// thousands character uses the firmware's unusual 0x39-based encoding.
[[nodiscard]] PlaybackNumericPair format_playback_numeric_pair(
    std::uint32_t value, std::uint32_t limit) noexcept;

} // namespace echo::audio
