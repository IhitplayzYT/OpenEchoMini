#include "echo/audio/playback_counter.hpp"

#include <array>

namespace echo::audio {
namespace {

constexpr std::uint16_t digit_glyph_base = 0xb3;
constexpr std::uint16_t colon_glyph = 0xbe;
constexpr std::uint16_t baseline_y = 0x9e;
constexpr std::uint16_t pair_colon_glyph = 0x12d;

void append_pair_value(std::array<std::uint8_t, 9>& characters,
                       std::size_t& count, std::uint32_t value) noexcept {
    const auto remainder = value % 1000;
    if (value > 999) {
        const auto thousands_digit = (value / 1000) % 10;
        const auto base = value < 10000 ? 0x30u : 0x39u;
        characters[count++] = static_cast<std::uint8_t>(base + thousands_digit);
    }
    if (value > 99) {
        characters[count++] = static_cast<std::uint8_t>(
            0x30u + remainder / 100);
    }
    const auto low_hundreds = remainder % 100;
    if (value > 9) {
        characters[count++] = static_cast<std::uint8_t>(
            0x30u + low_hundreds / 10);
    }
    characters[count++] = static_cast<std::uint8_t>(
        0x30u + low_hundreds % 10);
}

void append_pair(std::array<std::uint16_t, 8>& glyphs, std::size_t& index,
                 std::uint32_t value) noexcept {
    value %= 100;
    glyphs[index++] = static_cast<std::uint16_t>(digit_glyph_base + value / 10);
    glyphs[index++] = static_cast<std::uint16_t>(digit_glyph_base + value % 10);
}

} // namespace

PlaybackTripleCounter format_playback_time_counter(
    std::uint32_t total_seconds,
    bool alternate_position) noexcept {
    std::array<std::uint16_t, 8> glyph_ids{};
    std::size_t index = 0;
    std::uint16_t x = alternate_position ? 0x106 : 0x14;
    append_pair(glyph_ids, index, total_seconds / 3600);
    glyph_ids[index++] = colon_glyph;
    append_pair(glyph_ids, index, (total_seconds % 3600) / 60);
    glyph_ids[index++] = colon_glyph;
    append_pair(glyph_ids, index, total_seconds % 60);

    PlaybackTripleCounter result{};
    std::uint16_t colon_adjustment = 0;
    for (std::size_t i = 0; i < glyph_ids.size(); ++i) {
        result.glyphs[i] = {glyph_ids[i],
                            static_cast<std::uint16_t>(x + i * 7 - colon_adjustment),
                            baseline_y};
        if (glyph_ids[i] == colon_glyph) colon_adjustment += 4;
    }
    return result;
}

PlaybackNumericPair format_playback_numeric_pair(std::uint32_t value,
                                                 std::uint32_t limit) noexcept {
    if (value > limit) value = limit;
    std::array<std::uint8_t, 9> characters{};
    std::size_t count = 0;
    append_pair_value(characters, count, value);
    characters[count++] = ':';
    append_pair_value(characters, count, limit);

    PlaybackNumericPair result{};
    result.glyph_count = static_cast<std::uint8_t>(count);
    std::uint16_t colon_adjustment = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const bool colon = characters[i] == ':';
        const auto glyph_id = colon
                                  ? pair_colon_glyph
                                  : static_cast<std::uint16_t>(
                                        characters[i] + 0xf3u);
        const auto x = static_cast<std::uint16_t>(
            0x42u + i * 8u - colon_adjustment);
        result.glyphs[i] = {glyph_id, x,
                            static_cast<std::uint16_t>(colon ? 6 : 4)};
        if (colon) colon_adjustment += 2;
    }
    return result;
}

} // namespace echo::audio
