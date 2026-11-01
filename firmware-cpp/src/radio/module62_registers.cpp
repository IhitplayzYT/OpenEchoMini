#include "echo/radio/module62_registers.hpp"

namespace echo::radio {

std::uint8_t module62_frequency_register_value(
    const std::uint32_t frequency_tenths_mhz,
    const std::uint16_t mode_bits) noexcept {
    constexpr std::uint64_t divide_by_ten_reciprocal = 0xcccccccdULL;
    const auto scaled_frequency = static_cast<std::uint32_t>(
        (divide_by_ten_reciprocal * frequency_tenths_mhz) >> 35);

    std::uint32_t offset{};
    switch (mode_bits & 0x0cU) {
    case 0x00: offset = 0x366; break;
    case 0x04:
    case 0x08: offset = 0x2f8; break;
    default: offset = 0x28a; break;
    }
    const auto divisor = (mode_bits & 0x03U) == 1 ? 2u : 1u;
    return static_cast<std::uint8_t>(((scaled_frequency - offset) / divisor) & 0xffU);
}

Module62RegisterBlock build_module62_register_block(
    const std::array<std::uint16_t, 4> words,
    const std::uint16_t low_nibble) noexcept {
    Module62RegisterBlock result{};
    result.words_after = words;
    result.words_after[3] = static_cast<std::uint16_t>(
        (words[3] & 0xfff0U) | (low_nibble & 0x000fU));

    result.payload = {
        static_cast<std::uint8_t>(words[0] >> 8),
        static_cast<std::uint8_t>(words[0]),
        static_cast<std::uint8_t>(words[1] >> 8),
        static_cast<std::uint8_t>(words[1] & 0x00efU),
        static_cast<std::uint8_t>(words[2] >> 8),
        static_cast<std::uint8_t>(words[2]),
        static_cast<std::uint8_t>(words[3] >> 8),
        static_cast<std::uint8_t>((words[3] & 0x00f0U) | (low_nibble & 0x000fU)),
    };
    return result;
}

Module62SeekProbeResult analyze_module62_seek_statuses(
    const std::array<std::uint8_t, 3> statuses) noexcept {
    if (statuses[0] < 0x0f) return {false, 1};
    if (statuses[1] >= 0x0f && statuses[0] <= statuses[1]) {
        return {false, 2};
    }
    const bool third_probe_passes = statuses[2] < 0x0f || statuses[0] > statuses[2];
    const bool accepted = third_probe_passes && ((statuses[0] & 1u) != 0);
    return {accepted, 3};
}

} // namespace echo::radio
