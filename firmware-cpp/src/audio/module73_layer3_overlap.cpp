#include "echo/audio/module73_layer3_overlap.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>

namespace echo::audio {

bool module73_layer3_overlap_and_output(
    const std::span<const std::int32_t, 36> transformed_block,
    const std::span<std::int32_t, 18> overlap_history,
    const std::span<std::int32_t> interleaved_output,
    const std::size_t channel_index) noexcept {
    constexpr std::size_t kSampleStrideWords = 32;
    constexpr std::size_t kSamples = 18;
    if (channel_index >= interleaved_output.size() ||
        (interleaved_output.size() - 1U - channel_index) / kSampleStrideWords <
            kSamples - 1U) {
        return false;
    }

    for (std::size_t i = 0; i < kSamples; ++i) {
        const auto sum = static_cast<std::uint32_t>(transformed_block[i]) +
                         static_cast<std::uint32_t>(overlap_history[i]);
        interleaved_output[channel_index + i * kSampleStrideWords] =
            std::bit_cast<std::int32_t>(sum);
    }

    // Preserve the firmware's ascending word-copy order for partial overlap.
    for (std::size_t i = 0; i < kSamples; ++i) {
        overlap_history[i] = transformed_block[18U + i];
    }
    return true;
}

} // namespace echo::audio
