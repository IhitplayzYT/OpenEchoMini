#pragma once

#include "echo/audio/module63_control.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace echo::audio {

enum class Module63StopCall : std::uint8_t {
    disable_capture_callback,
    clear_capture_event,
    stop_capture_event,
    configure_idle_mode,
    configure_stream,
    send_control_code,
};

struct Module63StopAction {
    Module63StopCall call{};
    std::array<std::uint32_t, 7> arguments{};
    std::uint8_t argument_count{};
};

struct Module63StopInput {
    std::uint32_t state_field_2c{};
    std::uint32_t state_field_10{};
    std::uint32_t selector{};
    std::uint32_t state_field_18{};
    std::uint8_t state_byte_24{};
};

struct Module63StopPlan {
    std::vector<Module63StopAction> actions;
    bool special_idle_path{};
    bool selector_valid{};
    std::uint8_t selected_control_code{};
};

// Plans FUN_03081592's callback shutdown and idle/selector branch.
[[nodiscard]] Module63StopPlan plan_module63_stop(
    const Module63StopInput& input);

} // namespace echo::audio
