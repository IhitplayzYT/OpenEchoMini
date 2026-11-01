#include "echo/audio/module80_pcm_interleave.hpp"

#include <limits>

namespace echo::audio {
namespace {

void write_le(std::uint8_t* out, std::int32_t sample, unsigned bytes) noexcept {
    const auto bits = static_cast<std::uint32_t>(sample);
    for (unsigned i = 0; i < bytes; ++i) {
        out[i] = static_cast<std::uint8_t>(bits >> (i * 8));
    }
}

} // namespace

std::size_t interleave_module80_pcm(std::span<const std::int32_t> left,
                                   std::span<const std::int32_t> right,
                                   std::size_t sample_count,
                                   unsigned channels,
                                   unsigned bits_per_sample,
                                   std::span<std::uint8_t> output) noexcept {
    if ((channels != 1 && channels != 2) ||
        (bits_per_sample != 16 && bits_per_sample != 24) ||
        sample_count > left.size() ||
        (channels == 2 && sample_count > right.size())) {
        return 0;
    }

    const std::size_t bytes_per_sample = bits_per_sample / 8;
    if (sample_count > std::numeric_limits<std::size_t>::max() /
                           (2 * bytes_per_sample)) {
        return 0;
    }
    const auto bytes_required = sample_count * 2 * bytes_per_sample;
    if (output.size() < bytes_required) return 0;

    for (std::size_t i = 0; i < sample_count; ++i) {
        auto* frame = output.data() + i * 2 * bytes_per_sample;
        write_le(frame, left[i], static_cast<unsigned>(bytes_per_sample));
        write_le(frame + bytes_per_sample,
                 channels == 2 ? right[i] : left[i],
                 static_cast<unsigned>(bytes_per_sample));
    }
    return bytes_required;
}

} // namespace echo::audio
