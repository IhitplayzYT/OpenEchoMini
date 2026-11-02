#pragma once

#include <array>
#include <cstdint>

namespace echo::runtime {

enum class Module0BlockIoLockActionKind : std::uint8_t {
    disable_global_interrupts,
    enable_global_interrupts,
    capture_and_disable_irq,
    disable_irq,
    restore_irq_if_previously_enabled,
    clear_deferred_pendsv_flag,
    pend_pendsv,
};

struct Module0BlockIoLockAction {
    Module0BlockIoLockActionKind kind{
        Module0BlockIoLockActionKind::disable_global_interrupts};
    std::uint8_t interrupt_number{};
};

struct Module0BlockIoLockState {
    // Observed module-0 globals at 0x0301c85a and 0x0301c858.
    std::uint8_t critical_depth{};
    std::uint8_t device_lock_depth{};
    // 0x03004810 gates IRQ 26/27 masking; its producer is not yet identified.
    bool irq_mask_gate{};
    bool irq26_was_enabled{};
    bool irq27_was_enabled{};
    // 0x0301c859 indicates a deferred PendSV request in the gated path.
    bool deferred_pendsv{};
};

struct Module0BlockIoLockPlan {
    Module0BlockIoLockState state{};
    std::array<Module0BlockIoLockAction, 6> actions{};
    std::uint8_t action_count{};
    bool spins_forever{};
};

// Models the observed recursive device-lock wrapper. It never changes host
// interrupt state or performs NVIC register writes.
[[nodiscard]] inline Module0BlockIoLockPlan module0_block_io_lock_enter(
    Module0BlockIoLockState state) noexcept {
    Module0BlockIoLockPlan plan{state};
    const auto add = [&plan](Module0BlockIoLockActionKind kind,
                             std::uint8_t irq = 0) {
        plan.actions[plan.action_count++] = {kind, irq};
    };
    add(Module0BlockIoLockActionKind::disable_global_interrupts);
    ++plan.state.critical_depth;

    if (plan.state.device_lock_depth == 0) {
        if (!plan.state.irq_mask_gate) {
            add(Module0BlockIoLockActionKind::capture_and_disable_irq, 27);
            add(Module0BlockIoLockActionKind::capture_and_disable_irq, 26);
        }
    } else if (!plan.state.irq_mask_gate) {
        add(Module0BlockIoLockActionKind::disable_irq, 27);
        add(Module0BlockIoLockActionKind::disable_irq, 26);
    }

    ++plan.state.device_lock_depth;
    if (plan.state.device_lock_depth > 0xfa) {
        plan.spins_forever = true;
        return plan;
    }

    --plan.state.critical_depth;
    if (plan.state.critical_depth == 0) {
        add(Module0BlockIoLockActionKind::enable_global_interrupts);
    }
    return plan;
}

[[nodiscard]] inline Module0BlockIoLockPlan module0_block_io_lock_exit(
    Module0BlockIoLockState state) noexcept {
    Module0BlockIoLockPlan plan{state};
    const auto add = [&plan](Module0BlockIoLockActionKind kind,
                             std::uint8_t irq = 0) {
        plan.actions[plan.action_count++] = {kind, irq};
    };
    add(Module0BlockIoLockActionKind::disable_global_interrupts);
    ++plan.state.critical_depth;

    if (plan.state.device_lock_depth == 0) {
        plan.spins_forever = true;
        return plan;
    }
    --plan.state.device_lock_depth;

    if (plan.state.device_lock_depth == 0) {
        if (!plan.state.irq_mask_gate) {
            if (plan.state.irq27_was_enabled) {
                add(Module0BlockIoLockActionKind::restore_irq_if_previously_enabled, 27);
            }
            if (plan.state.irq26_was_enabled) {
                add(Module0BlockIoLockActionKind::restore_irq_if_previously_enabled, 26);
            }
        } else if (plan.state.deferred_pendsv) {
            plan.state.deferred_pendsv = false;
            add(Module0BlockIoLockActionKind::clear_deferred_pendsv_flag);
            add(Module0BlockIoLockActionKind::pend_pendsv);
        }
    }

    --plan.state.critical_depth;
    if (plan.state.critical_depth == 0) {
        add(Module0BlockIoLockActionKind::enable_global_interrupts);
    }
    return plan;
}

} // namespace echo::runtime
