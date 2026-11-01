#include "echo/display/module0_dirty_rows.hpp"

#include <algorithm>

namespace echo::display {

Module0DirtyRowSplit split_module0_dirty_rows(
    const std::uint16_t top,
    const std::uint16_t bottom) noexcept {
    constexpr std::uint16_t split_row = 0x8c;
    constexpr std::uint16_t last_row_before_split = split_row - 1;
    constexpr std::uint16_t last_column = 0x13f;

    Module0DirtyRowSplit result;
    if (top < split_row) {
        result.spans[result.count++] = {
            0, top, last_column, std::min(bottom, last_row_before_split)};
    }
    if (bottom > last_row_before_split) {
        result.spans[result.count++] = {
            0, std::max(top, split_row), last_column, bottom};
    }
    return result;
}

} // namespace echo::display
