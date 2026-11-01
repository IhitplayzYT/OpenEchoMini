#include "echo/apps/module32_setting_step.hpp"

namespace echo::apps {

Module32SettingStep step_module32_setting(
    std::uint8_t selection, const std::uint8_t maximum,
    const Module32SettingDirection direction, const bool query_ff_matches,
    const bool query_b6_matches) noexcept {
    if (direction == Module32SettingDirection::up) {
        if (selection < maximum) ++selection;
    } else if (selection != 0) {
        --selection;
    }

    return {selection, query_ff_matches, query_b6_matches, 0x3e, 1};
}

} // namespace echo::apps
