#include "echo/storage/module0_selection_navigation.hpp"

namespace echo::storage {

Module0SelectionNavigationPlan plan_module0_selection_navigation(
    const std::int32_t requested_steps) noexcept {
    if (requested_steps == 0) return {};

    const auto counter = static_cast<std::uint16_t>(requested_steps);
    if (requested_steps < 0) {
        const std::uint32_t steps = counter == 0
            ? 0x10000U
            : 0x10000U - static_cast<std::uint32_t>(counter);
        return {-1, steps, true};
    }

    const std::uint32_t steps = counter == 0
        ? 0x10000U
        : static_cast<std::uint32_t>(counter);
    return {1, steps, true};
}

} // namespace echo::storage
