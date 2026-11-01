#pragma once

#include <cstdint>

namespace echo::runtime {

enum class Module0InterruptActionKind : std::uint8_t {
    none,
    read_modify_set,
    read_modify_clear,
    write_one_to_set,
    write_one_to_clear,
};

struct Module0InterruptAction {
    Module0InterruptActionKind kind{Module0InterruptActionKind::none};
    std::uint32_t address{};
    std::uint32_t mask{};
};

// These return the MMIO action observed in module 0. They never perform it.
[[nodiscard]] Module0InterruptAction module0_interrupt_enable_action(
    std::uint32_t vector_index) noexcept;
[[nodiscard]] Module0InterruptAction module0_interrupt_disable_action(
    std::uint32_t vector_index) noexcept;
[[nodiscard]] Module0InterruptAction module0_interrupt_clear_pending_action(
    std::uint32_t vector_index) noexcept;

} // namespace echo::runtime
