#pragma once

#include <array>
#include <cstdint>
#include <array>

namespace echo::radio {

// Converts the requested frequency and mode bits to the module 62's
// one-byte tuner field. The result preserves the firmware's low-byte wrap.
[[nodiscard]] std::uint8_t module62_frequency_register_value(
    std::uint32_t frequency_tenths_mhz,
    std::uint16_t mode_bits) noexcept;

struct Module62RegisterBlock {
    std::array<std::uint16_t, 4> words_after{};
    std::array<std::uint8_t, 8> payload{};
};

// Builds the eight-byte write buffer from four 16-bit state words while
// preserving the observed cleared and replaced bit fields.
[[nodiscard]] Module62RegisterBlock build_module62_register_block(
    std::array<std::uint16_t, 4> words,
    std::uint16_t low_nibble) noexcept;

struct Module62SeekProbeResult {
    bool accepted{};
    std::uint8_t probes_used{};
};

// Evaluates the three status bytes observed by FUN_03094e86, at the requested
// frequency and the two adjacent 10-unit candidates.
[[nodiscard]] Module62SeekProbeResult analyze_module62_seek_statuses(
    std::array<std::uint8_t, 3> statuses) noexcept;

} // namespace echo::radio
