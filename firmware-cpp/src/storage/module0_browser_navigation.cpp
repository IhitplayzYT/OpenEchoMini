#include "echo/storage/module0_browser_navigation.hpp"

namespace echo::storage {

Module0BrowserSelectionMove plan_module0_browser_selection_move(
    const std::uint16_t current_index, const std::uint16_t item_count,
    const std::int32_t direction) noexcept {
    if (direction == 0) return {current_index, false, false, false};

    if (direction < 0) {
        const auto decremented = static_cast<std::uint16_t>(current_index - 1U);
        if (decremented == 0U) {
            return {item_count, true, false, true};
        }
        return {decremented, decremented != current_index, false, false};
    }

    const auto incremented = static_cast<std::uint16_t>(current_index + 1U);
    if (item_count < incremented) {
        return {1U, true, true, false};
    }
    return {incremented, incremented != current_index, false, false};
}

} // namespace echo::storage
