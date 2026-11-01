#include "echo/recording/module29_mode_step.hpp"

namespace echo::recording {

Module29ModeStep plan_module29_mode_step(
    const std::uint16_t current_state,
    const bool event_0x73_is_set) noexcept {
    if (event_0x73_is_set || current_state > 2) {
        return {};
    }

    Module29ModeStep result;
    result.handled = true;
    result.set_active_flag = true;
    switch (current_state) {
    case 0:
        result.next_state = 1;
        result.service_code = 0x7d;
        result.event_ids = {0x6a, 0x5b};
        result.event_count = 2;
        break;
    case 1:
        result.next_state = 2;
        result.service_code = 0x7e;
        result.event_ids[0] = 0x6b;
        result.event_count = 1;
        break;
    case 2:
        result.next_state = 1;
        result.service_code = 0x7f;
        result.event_ids[0] = 0x6a;
        result.event_count = 1;
        break;
    }
    return result;
}

} // namespace echo::recording
