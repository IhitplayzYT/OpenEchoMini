#pragma once

#include <cstdint>

namespace echo::storage {

struct Module0BrowserSelectionMove {
    std::uint16_t selected_index{};
    bool moved{};
    bool wrapped_forward{};
    bool wrapped_backward{};
};

// Models the one-step, one-based navigation branch in FUN_0305dfaa when
// param_1[6] is not 1. The firmware narrows the increment/decrement to 16 bits.
[[nodiscard]] Module0BrowserSelectionMove plan_module0_browser_selection_move(
    std::uint16_t current_index, std::uint16_t item_count,
    std::int32_t direction) noexcept;

} // namespace echo::storage
