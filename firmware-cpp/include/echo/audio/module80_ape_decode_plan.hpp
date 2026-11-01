#pragma once

#include <cstdint>

namespace echo::audio {

enum class Module80ApeDecodeRoute {
    mono_residual,
    mono_with_channel_filter,
    stereo_residual_passthrough,
    stereo_predictor,
};

struct Module80ApeDecodeChunkPlan {
    std::uint32_t sample_count{};
    Module80ApeDecodeRoute route{Module80ApeDecodeRoute::mono_residual};
};

// Models FUN_0100368e's 0x1000-sample chunk cap and its channel/mode branch.
[[nodiscard]] constexpr Module80ApeDecodeChunkPlan
plan_module80_ape_decode_chunk(std::uint32_t remaining_samples,
                               std::uint32_t channels,
                               std::uint32_t mode_flags) noexcept {
    const auto count = remaining_samples > 0x1000 ? 0x1000 : remaining_samples;
    if (channels == 1 || (mode_flags & 0x04) != 0) {
        return {count, (mode_flags & 0x03) == 0
                           ? Module80ApeDecodeRoute::mono_with_channel_filter
                           : Module80ApeDecodeRoute::mono_residual};
    }
    return {count, (mode_flags & 0x03) == 0x03
                       ? Module80ApeDecodeRoute::stereo_residual_passthrough
                       : Module80ApeDecodeRoute::stereo_predictor};
}

} // namespace echo::audio
