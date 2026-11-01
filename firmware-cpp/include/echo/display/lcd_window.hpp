#pragma once

#include <array>
#include <cstdint>

namespace echo::display {

struct LcdWindow {
    std::int32_t x_start{};
    std::int32_t y_start{};
    std::int32_t x_end{};
    std::int32_t y_end{};
};

// Encodes the 0x2A/0x2B/0x2C command sequence observed in module 4.
// The firmware adds 0x23 to both Y coordinates and transmits low 16 bits.
[[nodiscard]] std::array<std::uint8_t, 11> encode_lcd_window(
    const LcdWindow& window) noexcept;

} // namespace echo::display
