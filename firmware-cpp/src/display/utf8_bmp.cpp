#include "echo/display/utf8_bmp.hpp"

namespace echo::display {

bool decode_utf8_bmp(
    std::span<const std::uint8_t> input,
    std::vector<char16_t>& output) {
    output.clear();
    for (std::size_t i = 0; i < input.size();) {
        const auto first = input[i];
        if (first < 0x80) {
            output.push_back(static_cast<char16_t>(first));
            ++i;
            continue;
        }

        if (first >= 0xc2 && first <= 0xdf) {
            if (i + 1 >= input.size() || (input[i + 1] & 0xc0) != 0x80) {
                output.clear();
                return false;
            }
            const auto value = static_cast<std::uint16_t>(
                ((first & 0x1f) << 6) | (input[i + 1] & 0x3f));
            output.push_back(static_cast<char16_t>(value));
            i += 2;
            continue;
        }

        if (first >= 0xe0 && first <= 0xef) {
            if (i + 2 >= input.size() || (input[i + 1] & 0xc0) != 0x80 ||
                (input[i + 2] & 0xc0) != 0x80 ||
                (first == 0xe0 && input[i + 1] < 0xa0) ||
                (first == 0xed && input[i + 1] >= 0xa0)) {
                output.clear();
                return false;
            }
            const auto value = static_cast<std::uint16_t>(
                ((first & 0x0f) << 12) | ((input[i + 1] & 0x3f) << 6) |
                (input[i + 2] & 0x3f));
            output.push_back(static_cast<char16_t>(value));
            i += 3;
            continue;
        }

        output.clear();
        return false;
    }
    return true;
}

} // namespace echo::display
