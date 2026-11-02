#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace echo::audio {

struct Module0AudioDmaPackResult {
    std::uint32_t sample_slots{};
    std::size_t output_bytes{};
};

[[nodiscard]] inline std::uint32_t module0_audio_load_le32(
    std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

inline void module0_audio_store_le32(
    std::span<std::uint8_t> bytes, std::size_t offset,
    std::uint32_t value) noexcept {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 8U);
    bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 16U);
    bytes[offset + 3U] = static_cast<std::uint8_t>(value >> 24U);
}

[[nodiscard]] inline std::uint32_t module0_audio_arithmetic_shift8(
    std::uint32_t value) noexcept {
    return (value >> 8U) | ((value & 0x80000000U) ? 0xff000000U : 0U);
}

// Models FUN_03066938's width-specific input conversion: 16-bit samples are
// sign-extended and shifted left 8, 24-bit samples are sign-extended, and
// 32-bit samples are arithmetically shifted right 8.
[[nodiscard]] inline std::optional<Module0AudioDmaPackResult>
normalize_module0_audio_dma_samples(
    std::span<std::uint8_t> buffer,
    std::uint32_t logical_sample_count,
    std::uint32_t bit_depth) noexcept {
    if (logical_sample_count > std::numeric_limits<std::uint32_t>::max() / 2U ||
        (bit_depth != 16 && bit_depth != 24 && bit_depth != 32)) {
        return std::nullopt;
    }

    const auto slots = logical_sample_count * 2U;
    if (static_cast<std::uint64_t>(slots) >
        std::numeric_limits<std::size_t>::max() / 4U) {
        return std::nullopt;
    }
    const auto width = static_cast<std::size_t>(bit_depth / 8U);
    const auto output_bytes = static_cast<std::size_t>(slots) * 4U;
    if (buffer.size() < output_bytes) return std::nullopt;

    for (std::size_t i = slots; i != 0; --i) {
        const auto source = (i - 1U) * width;
        std::uint32_t normalized{};
        if (bit_depth == 16) {
            const auto bits = static_cast<std::uint16_t>(buffer[source]) |
                (static_cast<std::uint16_t>(buffer[source + 1U]) << 8U);
            const auto signed_value = (bits & 0x8000U)
                ? (static_cast<std::uint32_t>(bits) | 0xffff0000U)
                : static_cast<std::uint32_t>(bits);
            normalized = signed_value << 8U;
        } else if (bit_depth == 24) {
            normalized = static_cast<std::uint32_t>(buffer[source]) |
                (static_cast<std::uint32_t>(buffer[source + 1U]) << 8U) |
                (static_cast<std::uint32_t>(buffer[source + 2U]) << 16U);
            if ((normalized & 0x00800000U) != 0) normalized |= 0xff000000U;
        } else {
            normalized = module0_audio_arithmetic_shift8(
                module0_audio_load_le32(buffer, source));
        }
        module0_audio_store_le32(buffer, (i - 1U) * 4U, normalized);
    }

    return Module0AudioDmaPackResult{slots, output_bytes};
}

// Models FUN_030668c0's in-place expansion for the channel-5 audio DMA path.
// Sixteen- and 24-bit values are shifted into the high bits of 32-bit words;
// 32-bit data is left as-is. The firmware doubles the sample-slot count.
[[nodiscard]] inline std::optional<Module0AudioDmaPackResult>
expand_module0_audio_dma_samples(
    std::span<std::uint8_t> buffer,
    std::uint32_t logical_sample_count,
    std::uint32_t bit_depth) noexcept {
    if (logical_sample_count > std::numeric_limits<std::uint32_t>::max() / 2U) {
        return std::nullopt;
    }
    if (bit_depth != 16 && bit_depth != 24 && bit_depth != 32) {
        return std::nullopt;
    }

    const auto slots = logical_sample_count * 2U;
    if (static_cast<std::uint64_t>(slots) >
        std::numeric_limits<std::size_t>::max() / 4U) {
        return std::nullopt;
    }
    const auto bytes_per_input_sample = static_cast<std::size_t>(bit_depth / 8U);
    const auto output_bytes = static_cast<std::size_t>(slots) * 4U;
    const auto input_bytes = static_cast<std::size_t>(slots) * bytes_per_input_sample;
    if (buffer.size() < output_bytes || input_bytes > output_bytes) {
        return std::nullopt;
    }

    if (bit_depth != 32) {
        for (std::size_t i = slots; i != 0; --i) {
            const auto source = (i - 1U) * bytes_per_input_sample;
            const auto destination = (i - 1U) * 4U;
            const auto low = buffer[source];
            const auto middle = buffer[source + 1U];

            buffer[destination] = 0;
            if (bit_depth == 16) {
                buffer[destination + 1U] = 0;
                buffer[destination + 2U] = low;
                buffer[destination + 3U] = middle;
            } else {
                buffer[destination + 1U] = low;
                buffer[destination + 2U] = middle;
                buffer[destination + 3U] = buffer[source + 2U];
            }
        }
    }

    return Module0AudioDmaPackResult{slots, output_bytes};
}

} // namespace echo::audio
