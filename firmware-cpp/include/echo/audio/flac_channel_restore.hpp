#pragma once

#include "echo/audio/flac_frame_header.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace echo::audio {

struct FlacRestoredSample {
    std::array<std::int32_t, 2> channels{};
    std::uint8_t channel_count{};
};

// Restores one sample pair after the module 78 decoder has produced its
// independent or decorrelated subframe samples. The second coded value is
// required for stereo assignments.
[[nodiscard]] std::optional<FlacRestoredSample> restore_flac_channels(
    FlacChannelAssignment assignment,
    std::int32_t first,
    std::optional<std::int32_t> second = std::nullopt) noexcept;

} // namespace echo::audio
