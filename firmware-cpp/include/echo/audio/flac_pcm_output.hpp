#pragma once

#include "echo/audio/flac_frame_header.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

// Packs module 78's reconstructed channel samples into its interleaved output
// span. The target truncates to the selected slot width; it does not clip.
[[nodiscard]] inline std::optional<std::size_t> pack_module78_flac_pcm(
    std::span<const std::int32_t> channel0,
    std::span<const std::int32_t> channel1,
    std::size_t sample_count,
    std::uint8_t source_channels,
    FlacChannelAssignment assignment,
    std::uint8_t bits_per_sample,
    std::span<std::uint8_t> output) noexcept {
    if (source_channels == 0 || source_channels > 2 || bits_per_sample == 0 ||
        bits_per_sample > 32 ||
        sample_count > channel0.size() ||
        (source_channels == 2 && sample_count > channel1.size()) ||
        (assignment != FlacChannelAssignment::independent_mono && source_channels != 2)) {
        return std::nullopt;
    }

    const std::size_t bytes_per_sample = bits_per_sample <= 16 ? 2 :
        (bits_per_sample == 32 ? 4 : 3);
    if (sample_count > output.size() / (2 * bytes_per_sample)) {
        return std::nullopt;
    }

    const auto wrap = [](std::uint32_t value) noexcept {
        return std::bit_cast<std::int32_t>(value);
    };
    const auto store = [&](std::size_t at, std::int32_t sample) noexcept {
        const auto bits = static_cast<std::uint32_t>(sample);
        for (std::size_t byte = 0; byte < bytes_per_sample; ++byte) {
            output[at + byte] = static_cast<std::uint8_t>(bits >> (byte * 8));
        }
    };

    for (std::size_t i = 0; i < sample_count; ++i) {
        const std::uint32_t first = static_cast<std::uint32_t>(channel0[i]);
        std::uint32_t left = first;
        std::uint32_t right = first;
        switch (assignment) {
        case FlacChannelAssignment::independent_mono:
            if (source_channels == 2) {
                right = static_cast<std::uint32_t>(channel1[i]);
            }
            break;
        case FlacChannelAssignment::independent_stereo:
            left = first;
            right = static_cast<std::uint32_t>(channel1[i]);
            break;
        case FlacChannelAssignment::left_side_stereo:
            left = first;
            right = first - static_cast<std::uint32_t>(channel1[i]);
            break;
        case FlacChannelAssignment::side_right_stereo:
            right = static_cast<std::uint32_t>(channel1[i]);
            left = first + right;
            break;
        case FlacChannelAssignment::mid_side_stereo: {
            const auto side = static_cast<std::int32_t>(channel1[i]);
            const auto mid = static_cast<std::int32_t>(channel0[i]);
            const auto restored_left = static_cast<std::uint32_t>(mid - (side >> 1));
            left = restored_left;
            right = restored_left + static_cast<std::uint32_t>(side);
            break;
        }
        }
        const std::size_t frame = i * 2 * bytes_per_sample;
        store(frame, wrap(left));
        store(frame + bytes_per_sample, wrap(right));
    }
    return sample_count * 2 * bytes_per_sample;
}

} // namespace echo::audio
