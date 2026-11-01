#pragma once

#include <cstdint>
#include <span>

namespace echo::audio {

// Applies the observed 36-value long, start, or stop block scaling layout.
// Unknown block types preserve the input, matching FUN_01005648's fallthrough.
void apply_module73_layer3_block_scale(
    std::span<std::int32_t, 36> spectral_values,
    std::uint8_t block_type) noexcept;

} // namespace echo::audio
