#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace echo::audio {

// Reproduces FUN_0100ad6e's strided copy around the caller's source center.
// The native helper emits one quarter of sample_count values at a stride of
// two. A quarter-count congruent to zero mod two starts two values before the
// center; an odd quarter-count starts one value after it.
[[nodiscard]] inline std::optional<std::size_t> copy_vorbis_overlap_history(
    std::span<const std::int32_t> source,
    std::size_t source_center,
    std::uint32_t sample_count,
    std::span<std::int32_t> destination) noexcept {
    if (sample_count < 4) return std::size_t{0};
    const auto output_count = static_cast<std::size_t>(sample_count / 4U);
    if (destination.size() < output_count) return std::nullopt;

    std::size_t source_start = 0;
    if ((output_count & 1U) != 0) {
        if (source_center == std::numeric_limits<std::size_t>::max())
            return std::nullopt;
        source_start = source_center + 1U;
    } else {
        if (source_center < 2U) return std::nullopt;
        source_start = source_center - 2U;
    }
    if (source_start >= source.size() ||
        output_count - 1U > (source.size() - 1U - source_start) / 2U) {
        return std::nullopt;
    }

    for (std::size_t i = 0; i < output_count; ++i)
        destination[i] = source[source_start + i * 2U];
    return output_count;
}

} // namespace echo::audio
