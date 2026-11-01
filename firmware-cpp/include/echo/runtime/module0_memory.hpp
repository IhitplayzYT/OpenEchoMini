#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

namespace echo::runtime {

// Module 0's 0x03055d06 loop fills exactly the requested byte count.
void fill_module0_bytes(std::span<std::uint8_t> destination,
                        std::uint8_t value) noexcept;

// Models the overlap-safe range copy. Both offsets address this same backing
// memory, matching the firmware's memmove-style helper.
[[nodiscard]] bool move_module0_bytes(std::span<std::uint8_t> memory,
                                      std::size_t destination_offset,
                                      std::size_t source_offset,
                                      std::size_t count) noexcept;

// Models the target's bounded strncpy-like helper. It copies at most the
// destination size and zero-pads after a source terminator. Returns false
// without changing the destination when a short source span has no terminator.
[[nodiscard]] bool copy_module0_bounded_string(
    std::span<std::uint8_t> destination,
    std::span<const std::uint8_t> source) noexcept;

} // namespace echo::runtime
