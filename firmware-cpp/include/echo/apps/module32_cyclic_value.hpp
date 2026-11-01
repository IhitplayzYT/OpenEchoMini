#pragma once

#include <cstdint>

namespace echo::apps {

enum class Module32ValueDirection : std::uint8_t { down, up };

struct Module32CyclicValueStep {
    std::uint16_t value{};
    bool changed{};
};

// Models the common wrap arithmetic in FUN_0307e826. The caller supplies the
// two per-setting values read from the firmware's limit record.
[[nodiscard]] Module32CyclicValueStep step_module32_cyclic_value(
    std::uint16_t current, std::uint16_t upper_wrap_threshold,
    std::uint16_t lower_wrap_value,
    Module32ValueDirection direction) noexcept;

} // namespace echo::apps
