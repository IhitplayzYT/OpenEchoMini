#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

// Computes the module 73 MSB-first CRC recurrence over a bounded bit range.
// The initial value is supplied by the caller, as in FUN_010054ae.
[[nodiscard]] std::optional<std::uint16_t> module73_mpeg_crc16_bits(
    std::span<const std::uint8_t> bytes,
    std::size_t first_bit,
    std::size_t bit_count,
    std::uint16_t initial_value) noexcept;

} // namespace echo::audio
