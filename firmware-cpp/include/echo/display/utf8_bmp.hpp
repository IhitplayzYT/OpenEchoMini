#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace echo::display {

// Decodes the one-, two-, and three-byte UTF-8 subset used by the firmware's
// Bluetooth device-name renderer. Four-byte code points are unsupported.
[[nodiscard]] bool decode_utf8_bmp(
    std::span<const std::uint8_t> input,
    std::vector<char16_t>& output);

} // namespace echo::display
