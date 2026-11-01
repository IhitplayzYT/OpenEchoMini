#pragma once

#include <cstdint>
#include <optional>

namespace echo::audio {

// Returns the firmware's 22-entry preflag adjustment table value.
[[nodiscard]] std::optional<std::uint8_t>
module73_layer3_preflag_adjustment(unsigned scalefactor_band) noexcept;

// Computes the exponent passed to FUN_010055cc for one scalefactor band.
// preflag_adjustment must be zero when the granule's preflag does not apply.
[[nodiscard]] std::optional<std::int32_t> module73_layer3_scale_exponent(
    std::uint8_t global_gain,
    std::uint8_t scalefactor,
    bool scalefac_scale,
    std::uint8_t preflag_adjustment,
    std::uint8_t subblock_gain = 0) noexcept;

} // namespace echo::audio
