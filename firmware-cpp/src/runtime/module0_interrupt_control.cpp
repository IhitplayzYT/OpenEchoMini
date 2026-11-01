#include "echo/runtime/module0_interrupt_control.hpp"

namespace echo::runtime {
namespace {

constexpr Module0InterruptAction no_action{};

constexpr std::uint32_t exception_enable_mask(
    const std::uint32_t vector_index) noexcept {
    switch (vector_index) {
    case 4: return 1U << 16U;
    case 5: return 1U << 17U;
    case 6: return 1U << 18U;
    default: return 0;
    }
}

constexpr Module0InterruptAction external_irq_action(
    const std::uint32_t vector_index,
    const std::uint32_t register_base,
    const Module0InterruptActionKind kind) noexcept {
    const auto irq = vector_index - 16U;
    const auto address = register_base + ((irq >> 5U) * 4U);
    const auto mask = 1U << (irq & 31U);
    return {kind, address, mask};
}

} // namespace

Module0InterruptAction module0_interrupt_enable_action(
    const std::uint32_t vector_index) noexcept {
    const auto fault_mask = exception_enable_mask(vector_index);
    if (fault_mask != 0) {
        return {Module0InterruptActionKind::read_modify_set, 0xe000ed24U,
                fault_mask};
    }
    if (vector_index == 15) {
        return {Module0InterruptActionKind::read_modify_set, 0xe000ed34U, 2U};
    }
    if (vector_index >= 16) {
        return external_irq_action(vector_index, 0xe000e100U,
                                   Module0InterruptActionKind::write_one_to_set);
    }
    return no_action;
}

Module0InterruptAction module0_interrupt_disable_action(
    const std::uint32_t vector_index) noexcept {
    const auto fault_mask = exception_enable_mask(vector_index);
    if (fault_mask != 0) {
        return {Module0InterruptActionKind::read_modify_clear, 0xe000ed24U,
                fault_mask};
    }
    if (vector_index == 15) {
        return {Module0InterruptActionKind::read_modify_clear, 0xe000ed34U, 2U};
    }
    if (vector_index >= 16) {
        return external_irq_action(vector_index, 0xe000e180U,
                                   Module0InterruptActionKind::write_one_to_clear);
    }
    return no_action;
}

Module0InterruptAction module0_interrupt_clear_pending_action(
    const std::uint32_t vector_index) noexcept {
    if (vector_index == 14) {
        return {Module0InterruptActionKind::read_modify_set, 0xe000ed04U,
                0x08000000U};
    }
    if (vector_index == 15) {
        return {Module0InterruptActionKind::read_modify_set, 0xe000ed04U,
                0x02000000U};
    }
    if (vector_index >= 16) {
        return external_irq_action(vector_index, 0xe000e280U,
                                   Module0InterruptActionKind::write_one_to_clear);
    }
    return no_action;
}

} // namespace echo::runtime
