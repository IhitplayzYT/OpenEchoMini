#pragma once

#include <array>
#include <cstdint>

namespace echo::runtime {

// Four fixed-width register lines emitted by module 0's shared exception
// handler, excluding any UART timing or device access.
using Module0ExceptionDump = std::array<char, 54>;

[[nodiscard]] Module0ExceptionDump format_module0_exception_dump(
    std::uint32_t stacked_pc,
    std::uint32_t stacked_lr,
    std::uint32_t main_stack_pointer,
    std::uint32_t process_stack_pointer) noexcept;

} // namespace echo::runtime
