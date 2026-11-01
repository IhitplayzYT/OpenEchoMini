#pragma once

#include <cstdint>

namespace echo::apps {

enum class Module32SettingDirection : std::uint8_t { down, up };

struct Module32SettingStep {
    std::uint8_t selection{};
    bool dispatch_handler_ff{};
    bool dispatch_handler_b6{};
    std::uint16_t redraw_event{0x3e};
    std::uint8_t return_value{1};
};

// Models FUN_0307d6e8 / FUN_0307d72c. Event-query matches are supplied by
// the caller because their shared menu state is outside this overlay slice.
[[nodiscard]] Module32SettingStep step_module32_setting(
    std::uint8_t selection, std::uint8_t maximum,
    Module32SettingDirection direction, bool query_ff_matches,
    bool query_b6_matches) noexcept;

} // namespace echo::apps
