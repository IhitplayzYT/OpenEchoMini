#pragma once

#include <cstdint>
#include <span>

namespace echo::audio {

// Models FUN_010057d8: 18 Q-scaled inputs become 36 windowed outputs.
void transform_module73_layer3_block(
    std::span<const std::int32_t, 18> spectral_values,
    std::span<std::int32_t, 36> output_values) noexcept;

} // namespace echo::audio
