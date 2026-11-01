#include "echo/container/module75_mov_atoms.hpp"

namespace echo::container {
namespace {

std::uint32_t read_be32(const std::uint8_t* bytes) noexcept {
    return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
           (static_cast<std::uint32_t>(bytes[1]) << 16U) |
           (static_cast<std::uint32_t>(bytes[2]) << 8U) |
           static_cast<std::uint32_t>(bytes[3]);
}

}  // namespace

Module75MovStatus parse_module75_mov_atom_header(
    const std::span<const std::uint8_t> bytes,
    const std::size_t offset,
    Module75MovAtomHeader& output) noexcept {
    constexpr std::size_t header_size = 8;
    if (offset > bytes.size() || header_size > bytes.size() - offset)
        return Module75MovStatus::input_error;

    const auto atom_size = read_be32(bytes.data() + offset);
    if (atom_size < header_size) return Module75MovStatus::invalid_atom;
    if (atom_size > bytes.size() - offset) return Module75MovStatus::input_error;

    const Module75MovAtomHeader parsed{
        atom_size,
        read_be32(bytes.data() + offset + 4),
        offset,
        offset + header_size,
        offset + atom_size};
    output = parsed;
    return Module75MovStatus::ok;
}

}  // namespace echo::container
