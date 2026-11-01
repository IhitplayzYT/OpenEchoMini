#include "echo/runtime/module0_irq_guard.hpp"

namespace echo::runtime {
namespace {

constexpr std::uint32_t nvic_iser0 = 0xe000e100U;
constexpr std::uint32_t nvic_icer0 = 0xe000e180U;
constexpr std::uint32_t irq5_mask = 1U << 5U;

void append(Module0IrqGuardPlan& plan,
            const Module0IrqGuardActionKind kind,
            const std::uint32_t address = 0,
            const std::uint32_t mask = 0) noexcept {
    if (plan.action_count < plan.actions.size()) {
        plan.actions[plan.action_count++] = {kind, address, mask};
    }
}

void enter_critical(Module0IrqGuardPlan& plan) noexcept {
    append(plan, Module0IrqGuardActionKind::disable_global_interrupts);
    plan.state.critical_depth =
        static_cast<std::uint8_t>(plan.state.critical_depth + 1U);
}

void exit_critical(Module0IrqGuardPlan& plan) noexcept {
    if (plan.state.critical_depth == 0) {
        plan.spins_forever = true;
        return;
    }

    plan.state.critical_depth =
        static_cast<std::uint8_t>(plan.state.critical_depth - 1U);
    if (plan.state.critical_depth == 0) {
        append(plan, Module0IrqGuardActionKind::enable_global_interrupts);
    }
}

} // namespace

Module0IrqGuardPlan module0_critical_enter(
    const Module0IrqGuardState state) noexcept {
    Module0IrqGuardPlan plan{};
    plan.state = state;
    enter_critical(plan);
    return plan;
}

Module0IrqGuardPlan module0_critical_exit(
    const Module0IrqGuardState state) noexcept {
    Module0IrqGuardPlan plan{};
    plan.state = state;
    append(plan, Module0IrqGuardActionKind::disable_global_interrupts);
    exit_critical(plan);
    return plan;
}

Module0IrqGuardPlan module0_irq5_mask_enter(
    const Module0IrqGuardState state) noexcept {
    Module0IrqGuardPlan plan{};
    plan.state = state;
    enter_critical(plan);
    append(plan, Module0IrqGuardActionKind::write_one_to_clear, nvic_icer0,
           irq5_mask);
    plan.state.irq5_mask_depth =
        static_cast<std::uint8_t>(plan.state.irq5_mask_depth + 1U);
    exit_critical(plan);
    return plan;
}

Module0IrqGuardPlan module0_irq5_mask_exit(
    const Module0IrqGuardState state) noexcept {
    Module0IrqGuardPlan plan{};
    plan.state = state;
    enter_critical(plan);
    if (plan.state.irq5_mask_depth == 0) {
        plan.spins_forever = true;
        return plan;
    }

    plan.state.irq5_mask_depth =
        static_cast<std::uint8_t>(plan.state.irq5_mask_depth - 1U);
    if (plan.state.irq5_mask_depth == 0) {
        append(plan, Module0IrqGuardActionKind::write_one_to_set, nvic_iser0,
               irq5_mask);
    }
    exit_critical(plan);
    return plan;
}

} // namespace echo::runtime
