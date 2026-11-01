#pragma once

#include <array>
#include <cstdint>

namespace echo::runtime {

enum class Module0IrqGuardActionKind : std::uint8_t {
    disable_global_interrupts,
    enable_global_interrupts,
    write_one_to_set,
    write_one_to_clear,
};

struct Module0IrqGuardAction {
    Module0IrqGuardActionKind kind{
        Module0IrqGuardActionKind::disable_global_interrupts};
    std::uint32_t address{};
    std::uint32_t mask{};
};

struct Module0IrqGuardState {
    // Firmware globals at 0x0301c85a and 0x0301c85b.
    std::uint8_t critical_depth{};
    std::uint8_t irq5_mask_depth{};
};

struct Module0IrqGuardPlan {
    Module0IrqGuardState state{};
    std::array<Module0IrqGuardAction, 3> actions{};
    std::uint8_t action_count{};
    // The firmware spins forever on an unmatched exit.
    bool spins_forever{};
};

// Pure plans for the observed module 0 nesting helpers. These never touch
// PRIMASK or NVIC registers.
[[nodiscard]] Module0IrqGuardPlan module0_critical_enter(
    Module0IrqGuardState state) noexcept;
[[nodiscard]] Module0IrqGuardPlan module0_critical_exit(
    Module0IrqGuardState state) noexcept;
[[nodiscard]] Module0IrqGuardPlan module0_irq5_mask_enter(
    Module0IrqGuardState state) noexcept;
[[nodiscard]] Module0IrqGuardPlan module0_irq5_mask_exit(
    Module0IrqGuardState state) noexcept;

} // namespace echo::runtime
