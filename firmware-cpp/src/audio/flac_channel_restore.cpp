#include "echo/audio/flac_channel_restore.hpp"

#include <limits>

namespace echo::audio {
namespace {

std::optional<std::int32_t> narrow_sample(const std::int64_t value) noexcept {
    if (value < std::numeric_limits<std::int32_t>::min()
        || value > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::int32_t>(value);
}

} // namespace

std::optional<FlacRestoredSample> restore_flac_channels(
    const FlacChannelAssignment assignment,
    const std::int32_t first,
    const std::optional<std::int32_t> second) noexcept {
    if (assignment == FlacChannelAssignment::independent_mono) {
        if (second) return std::nullopt;
        return FlacRestoredSample{{first, 0}, 1};
    }
    if (!second) return std::nullopt;

    switch (assignment) {
    case FlacChannelAssignment::independent_stereo:
        return FlacRestoredSample{{first, *second}, 2};
    case FlacChannelAssignment::left_side_stereo: {
        const auto right = narrow_sample(static_cast<std::int64_t>(first) - *second);
        if (!right) return std::nullopt;
        return FlacRestoredSample{{first, *right}, 2};
    }
    case FlacChannelAssignment::side_right_stereo: {
        const auto left = narrow_sample(static_cast<std::int64_t>(first) + *second);
        if (!left) return std::nullopt;
        return FlacRestoredSample{{*left, *second}, 2};
    }
    case FlacChannelAssignment::mid_side_stereo: {
        const auto side_half = *second >> 1;
        const auto left = narrow_sample(static_cast<std::int64_t>(first)
            + (static_cast<std::int64_t>(*second) - side_half));
        const auto right = narrow_sample(static_cast<std::int64_t>(first) - side_half);
        if (!left || !right) return std::nullopt;
        return FlacRestoredSample{{*left, *right}, 2};
    }
    case FlacChannelAssignment::independent_mono:
        break;
    }
    return std::nullopt;
}

} // namespace echo::audio
