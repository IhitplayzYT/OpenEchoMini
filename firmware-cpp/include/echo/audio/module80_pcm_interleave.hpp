#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::audio {

// Models FUN_01003e38's observed interleaved output layout. Samples are
// truncated to the low 16 or 24 bits; mono input is duplicated to both sides.
// Returns the number of bytes written, or zero for an invalid/short buffer.
[[nodiscard]] std::size_t interleave_module80_pcm(
    std::span<const std::int32_t> left,
    std::span<const std::int32_t> right,
    std::size_t sample_count,
    unsigned channels,
    unsigned bits_per_sample,
    std::span<std::uint8_t> output) noexcept;

} // namespace echo::audio
