#pragma once

#include <cstdint>

namespace echo::storage {

struct Module0SelectionNavigationPlan {
    // The direction passed to FUN_0305dfaa for each attempted step.
    std::int8_t direction{};
    // FUN_0305e756 uses a do/while over a signed 16-bit local counter.
    std::uint32_t maximum_steps{};
    // The firmware returns at the first step whose result is nonzero.
    bool stop_on_nonzero_result{};
};

// Models the repeated-call behavior of FUN_0305e756. The original 32-bit
// argument chooses direction and zero handling, while its low 16 bits seed
// the loop counter. Ordinary key-repeat counts produce their absolute value
// as the step count; unusual large values retain the observed 16-bit wrap.
[[nodiscard]] Module0SelectionNavigationPlan
plan_module0_selection_navigation(std::int32_t requested_steps) noexcept;

} // namespace echo::storage
