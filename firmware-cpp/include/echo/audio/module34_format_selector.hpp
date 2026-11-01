#pragma once

#include <array>
#include <cstdint>

namespace echo::audio {

struct Module34FormatSelectorResult {
    // The firmware reports a one-based position in its packed three-byte list.
    std::uint8_t matched_entry{};
    std::uint8_t callback_selector{0xff};
    bool matched{};
};

// Models module 34's exact three-byte token comparison. Input is kept raw so
// the caller controls case conversion, as the observed helper compares bytes.
[[nodiscard]] Module34FormatSelectorResult select_module34_format_callback(
    std::array<char, 3> token,
    std::uint8_t mode_state) noexcept;

} // namespace echo::audio
