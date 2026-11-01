#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::container {

enum class Module75MovStatus : std::int8_t {
    ok = 0,
    input_error = -5,
    invalid_atom = -8,
};

struct Module75MovAtomHeader {
    std::uint32_t size{};
    std::uint32_t type_fourcc{};
    std::size_t header_offset{};
    std::size_t payload_offset{};
    std::size_t end_offset{};
};

// Ports the fixed 32-bit size + 4-byte type prefix read by FUN_01005066.
// Extended-size and size-to-end atom forms are not inferred from this routine.
Module75MovStatus parse_module75_mov_atom_header(
    std::span<const std::uint8_t> bytes,
    std::size_t offset,
    Module75MovAtomHeader& output) noexcept;

}  // namespace echo::container
