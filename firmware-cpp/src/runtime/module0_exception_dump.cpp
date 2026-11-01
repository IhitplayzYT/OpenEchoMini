#include "echo/runtime/module0_exception_dump.hpp"

#include <cstddef>

namespace echo::runtime {
namespace {

constexpr char kHexDigits[] = "0123456789ABCDEF";

void append_line(Module0ExceptionDump& output,
                 const std::size_t offset,
                 const char* label,
                 const std::size_t label_length,
                 const std::uint32_t value) noexcept {
    for (std::size_t i = 0; i < label_length; ++i) {
        output[offset + i] = label[i];
    }
    output[offset + label_length] = ':';
    for (std::size_t digit = 0; digit < 8; ++digit) {
        const auto shift = static_cast<unsigned>((7U - digit) * 4U);
        output[offset + label_length + 1 + digit] =
            kHexDigits[(value >> shift) & 0xfU];
    }
    output[offset + label_length + 9] = '\r';
    output[offset + label_length + 10] = '\n';
}

} // namespace

Module0ExceptionDump format_module0_exception_dump(
    const std::uint32_t stacked_pc,
    const std::uint32_t stacked_lr,
    const std::uint32_t main_stack_pointer,
    const std::uint32_t process_stack_pointer) noexcept {
    Module0ExceptionDump output{};
    append_line(output, 0, "PC", 2, stacked_pc);
    append_line(output, 13, "LR", 2, stacked_lr);
    append_line(output, 26, "MSP", 3, main_stack_pointer);
    append_line(output, 40, "PSP", 3, process_stack_pointer);
    return output;
}

} // namespace echo::runtime
