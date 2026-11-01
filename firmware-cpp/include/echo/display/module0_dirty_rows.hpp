#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::display {

struct DirtyRowSpan {
    std::uint16_t x_begin = 0;
    std::uint16_t y_begin = 0;
    std::uint16_t x_end = 0;
    std::uint16_t y_end = 0;
};

struct Module0DirtyRowSplit {
    std::array<DirtyRowSpan, 2> spans{};
    std::size_t count = 0;
};

// Splits the inclusive firmware row range at y=140 (0x8c).
[[nodiscard]] Module0DirtyRowSplit split_module0_dirty_rows(
    std::uint16_t top,
    std::uint16_t bottom) noexcept;

} // namespace echo::display
