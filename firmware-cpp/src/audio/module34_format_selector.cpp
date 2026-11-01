#include "echo/audio/module34_format_selector.hpp"

#include <cstddef>

namespace echo::audio {
namespace {

constexpr std::array<std::array<char, 3>, 15> kTokens{{
    {{'M', 'P', '1'}}, {{'M', 'P', '2'}}, {{'M', 'P', '3'}},
    {{'W', 'M', 'A'}}, {{'W', 'A', 'V'}}, {{'A', 'P', 'E'}},
    {{'F', 'L', 'A'}}, {{'A', 'A', 'C'}}, {{'M', '4', 'A'}},
    {{'O', 'G', 'G'}}, {{'M', 'P', '4'}}, {{'3', 'G', 'P'}},
    {{'D', 'F', 'F'}}, {{'D', 'S', 'F'}}, {{'M', '3', 'U'}},
}};

} // namespace

Module34FormatSelectorResult select_module34_format_callback(
    std::array<char, 3> token,
    std::uint8_t mode_state) noexcept {
    std::uint8_t entry = 0;
    for (std::size_t i = 0; i < kTokens.size(); ++i) {
        if (kTokens[i] == token) {
            entry = static_cast<std::uint8_t>(i + 1);
            break;
        }
    }

    if (entry == 0) {
        return {0, 0xff, false};
    }

    switch (entry) {
    case 1: // MP1
    case 2: // MP2
    case 3: // MP3
        return {entry, 0, true};
    case 4: // WMA
        return {entry, 1, true};
    case 5: // WAV
        return {entry, 3, true};
    case 6: // APE
        return {entry, 5, true};
    case 7: // FLA
        return {entry, 6, true};
    case 8:  // AAC
    case 9:  // M4A
    case 11: // MP4
    case 12: // 3GP
        return {entry,
                static_cast<std::uint8_t>(mode_state != 7 && mode_state == 0x0d
                                              ? 7
                                              : 2),
                true};
    case 10: // OGG
        return {entry, 4, true};
    case 13: // DFF
        return {entry, 8, true};
    case 14: // DSF
        return {entry, 9, true};
    default: // M3U is present in the token table but has no selector case.
        return {entry, 0xff, true};
    }
}

} // namespace echo::audio
