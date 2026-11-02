#pragma once

#include <cstdint>
#include <span>

namespace echo::storage {

// Maps one code point through module 0's compact case table, FUN_03058f22.
[[nodiscard]] std::uint32_t map_module0_name_codepoint(
    std::uint32_t codepoint) noexcept;

// Reproduces FUN_03058fde for the narrow-name path: each 16-bit unit is
// examined by its low byte, mapped, folded into the 16-bit rotate/add value,
// then rotated once more for the zero high byte. Stops on a zero low byte.
[[nodiscard]] std::uint16_t hash_module0_directory_name(
    std::span<const std::uint16_t> nul_terminated_name) noexcept;

} // namespace echo::storage
