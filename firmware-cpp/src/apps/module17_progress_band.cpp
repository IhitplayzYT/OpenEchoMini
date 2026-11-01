#include "echo/apps/module17_progress_band.hpp"

namespace echo::apps {

Module17ProgressBandDraw plan_module17_progress_band(
    const Module17ProgressBandInput& input) noexcept {
    if (input.event_16e) return {};

    const Module17ProgressBandRecord* selected = nullptr;
    std::uint32_t event_id = 0;
    if (input.event_16b) {
        selected = &input.band_16b;
        event_id = 0x16b;
    } else if (input.event_16d) {
        selected = &input.band_16d;
        event_id = 0x16d;
    } else {
        return {};
    }

    // The firmware performs this subtraction in 32-bit unsigned arithmetic.
    if (static_cast<std::uint32_t>(selected->current - selected->start) >=
        selected->span) {
        return {};
    }

    const auto width = selected->record_selector == input.shared_comparison_value
        ? input.shared_comparison_value
        : input.alternate_width;
    return {
        true,
        event_id,
        input.draw_base,
        static_cast<std::uint16_t>(selected->current + selected->origin),
        static_cast<std::uint16_t>(width),
        1,
    };
}

} // namespace echo::apps
