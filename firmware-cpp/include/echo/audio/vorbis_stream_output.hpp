#pragma once

#include "echo/audio/vorbis_output.hpp"
#include "echo/audio/vorbis_window_tables.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

// Renders the recovered overlap transition for every channel into one
// interleaved s16 output span. The caller supplies the two neighboring block
// window decisions from decoder state; mode-to-neighbor flag ownership is
// intentionally kept at that boundary.
[[nodiscard]] inline std::optional<std::size_t> render_vorbis_overlap_channels(
    std::uint32_t first_block_size,
    std::uint32_t second_block_size,
    bool first_window_long,
    bool second_window_long,
    std::span<const std::vector<std::int32_t>> first_channel_spectra,
    std::span<const std::vector<std::int32_t>> second_channel_spectra,
    std::uint32_t requested_begin,
    std::uint32_t requested_end,
    std::span<std::int16_t> interleaved_output,
    std::size_t output_frame_offset) noexcept {
    if (first_channel_spectra.empty() ||
        first_channel_spectra.size() != second_channel_spectra.size()) {
        return std::nullopt;
    }
    const auto first_window = vorbis_window_coefficients(first_block_size);
    const auto second_window = vorbis_window_coefficients(second_block_size);
    if (!first_window || !second_window) return std::nullopt;

    std::optional<std::size_t> rendered_frames;
    for (std::size_t channel = 0; channel < first_channel_spectra.size(); ++channel) {
        if (channel > std::numeric_limits<std::size_t>::max() - output_frame_offset)
            return std::nullopt;
        const auto rendered = render_vorbis_overlap_channel(
            first_block_size, second_block_size, first_window_long,
            second_window_long, first_channel_spectra[channel],
            second_channel_spectra[channel], *first_window, *second_window,
            requested_begin, requested_end, interleaved_output,
            output_frame_offset + channel, first_channel_spectra.size());
        if (!rendered || (rendered_frames && *rendered_frames != *rendered))
            return std::nullopt;
        rendered_frames = *rendered;
    }
    return rendered_frames;
}

} // namespace echo::audio
