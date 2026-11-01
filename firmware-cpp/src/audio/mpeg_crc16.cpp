#include "echo/audio/mpeg_crc16.hpp"

#include <limits>

namespace echo::audio {

std::optional<std::uint16_t> module73_mpeg_crc16_bits(
    const std::span<const std::uint8_t> bytes,
    const std::size_t first_bit,
    const std::size_t bit_count,
    const std::uint16_t initial_value) noexcept {
    if (bytes.size() > std::numeric_limits<std::size_t>::max() / 8U) {
        return std::nullopt;
    }
    const std::size_t available_bits = bytes.size() * 8U;
    if (first_bit > available_bits || bit_count > available_bits - first_bit) {
        return std::nullopt;
    }

    std::uint16_t crc = initial_value;
    for (std::size_t bit = 0; bit < bit_count; ++bit) {
        const std::size_t position = first_bit + bit;
        const auto input_bit = static_cast<std::uint16_t>(
            (bytes[position / 8U] >> (7U - position % 8U)) & 1U);
        const bool feedback = ((crc >> 15U) & 1U) != input_bit;
        crc = static_cast<std::uint16_t>(crc << 1U);
        if (feedback) crc = static_cast<std::uint16_t>(crc ^ 0x8005U);
    }
    return crc;
}

} // namespace echo::audio
