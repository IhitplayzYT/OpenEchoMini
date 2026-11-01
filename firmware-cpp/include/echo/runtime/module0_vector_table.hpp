#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::runtime {

inline constexpr std::size_t kModule0VectorCount = 57;
inline constexpr std::uint32_t kModule0VectorTableAddress = 0x03004000U;
inline constexpr std::uint32_t kModule0InitialStackPointer = 0x03004000U;
inline constexpr std::uint32_t kModule0ResetHandler = 0x0305655dU;
inline constexpr std::uint32_t kModule0DefaultExceptionHandler = 0x0306814fU;

struct Module0VectorTable {
    std::array<std::uint32_t, kModule0VectorCount> entries{};
    std::uint32_t vtor{};
};

void initialize_module0_vector_table(Module0VectorTable& table) noexcept;

// Matches the firmware's index check (0..56). If VTOR does not point at the
// table address, the table is initialized before the requested entry is set.
[[nodiscard]] bool set_module0_vector_handler(
    Module0VectorTable& table,
    std::size_t index,
    std::uint32_t handler_address) noexcept;

// Assigns the shared exception reporter to one valid vector slot.
[[nodiscard]] bool set_module0_vector_default(
    Module0VectorTable& table,
    std::size_t index) noexcept;

} // namespace echo::runtime
