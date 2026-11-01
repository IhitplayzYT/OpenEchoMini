#include "echo/display/lcd_window.hpp"

#include <cstddef>

namespace echo::display {
namespace {

void append_be16(const std::uint16_t value,
                 std::array<std::uint8_t, 11>& output,
                 const std::size_t offset) noexcept {
    output[offset] = static_cast<std::uint8_t>(value >> 8U);
    output[offset + 1U] = static_cast<std::uint8_t>(value & 0xffU);
}

std::uint16_t low16(const std::int32_t value, const std::int64_t adjustment = 0) noexcept {
    const auto adjusted = static_cast<std::uint64_t>(static_cast<std::int64_t>(value) + adjustment);
    return static_cast<std::uint16_t>(adjusted & 0xffffU);
}

} // namespace

std::array<std::uint8_t, 11> encode_lcd_window(const LcdWindow& window) noexcept {
    std::array<std::uint8_t, 11> bytes{};
    bytes[0] = 0x2a;
    append_be16(low16(window.x_start), bytes, 1);
    append_be16(low16(window.x_end), bytes, 3);
    bytes[5] = 0x2b;
    append_be16(low16(window.y_start, 0x23), bytes, 6);
    append_be16(low16(window.y_end, 0x23), bytes, 8);
    bytes[10] = 0x2c;
    return bytes;
}

} // namespace echo::display
