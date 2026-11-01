#include "echo/apps/module32_cyclic_value.hpp"

namespace echo::apps {

Module32CyclicValueStep step_module32_cyclic_value(
    const std::uint16_t current, const std::uint16_t upper_wrap_threshold,
    const std::uint16_t lower_wrap_value,
    const Module32ValueDirection direction) noexcept {
    std::uint16_t next = current;
    if (direction == Module32ValueDirection::up) {
        next = current < upper_wrap_threshold
            ? static_cast<std::uint16_t>(current + 1)
            : lower_wrap_value;
    } else {
        next = lower_wrap_value < current
            ? static_cast<std::uint16_t>(current - 1)
            : upper_wrap_threshold;
    }
    return {next, next != current};
}

} // namespace echo::apps
