#pragma once

#include <cstdint>
#include <optional>

namespace echo::audio {

// FUN_01005bba converts milliseconds to the module's media-time units using
// quotient/remainder arithmetic and the parsed time scale. Unsigned operations
// retain the firmware's 32-bit wrap behavior. A zero scale is rejected.
[[nodiscard]] inline std::optional<std::uint32_t> module79_time_to_media_units(
    std::uint32_t time_ms, std::uint32_t time_scale) noexcept {
    if (time_scale == 0) return std::nullopt;
    return (time_ms / 1000U) * time_scale +
           (time_scale * (time_ms % 1000U)) / 1000U;
}

} // namespace echo::audio
