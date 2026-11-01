#include "echo/runtime/module0_vector_table.hpp"

#include <algorithm>

namespace echo::runtime {

void initialize_module0_vector_table(Module0VectorTable& table) noexcept {
    table.entries[0] = kModule0InitialStackPointer;
    table.entries[1] = kModule0ResetHandler;
    std::fill(table.entries.begin() + 2, table.entries.end(),
              kModule0DefaultExceptionHandler);
    table.vtor = kModule0VectorTableAddress;
}

bool set_module0_vector_handler(Module0VectorTable& table,
                                const std::size_t index,
                                const std::uint32_t handler_address) noexcept {
    if (index >= kModule0VectorCount) return false;
    if (table.vtor != kModule0VectorTableAddress) {
        initialize_module0_vector_table(table);
    }
    table.entries[index] = handler_address;
    return true;
}

bool set_module0_vector_default(Module0VectorTable& table,
                                const std::size_t index) noexcept {
    if (index >= kModule0VectorCount) return false;
    table.entries[index] = kModule0DefaultExceptionHandler;
    return true;
}

} // namespace echo::runtime
