#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::runtime {

struct Module0RegisterWrite {
    std::uint32_t address;
    std::uint32_t value;
};

struct Module0RestartPlan {
    std::array<Module0RegisterWrite, 12> register_writes;
    std::uint32_t zero_fill_entry;
    std::uint32_t zero_fill_address;
    std::uint32_t zero_fill_length;
    std::uint32_t vector_table_address;
};

// Describes the observed module 0 reboot routine. This is a data model only;
// it does not perform MMIO writes, clear host memory, or restart a device.
[[nodiscard]] const Module0RestartPlan& module0_restart_plan() noexcept;

} // namespace echo::runtime
