#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::recording {

struct Module29ModeStep {
    bool handled = false;
    std::uint16_t next_state = 0;
    bool set_active_flag = false;
    std::uint8_t service_code = 0;
    std::array<std::uint16_t, 2> event_ids{};
    std::size_t event_count = 0;
};

// Models only module 29's gated three-state branch. State labels and service
// meanings are intentionally left as firmware values.
[[nodiscard]] Module29ModeStep plan_module29_mode_step(
    std::uint16_t current_state,
    bool event_0x73_is_set) noexcept;

} // namespace echo::recording
