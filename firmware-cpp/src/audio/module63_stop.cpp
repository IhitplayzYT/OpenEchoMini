#include "echo/audio/module63_stop.hpp"

namespace echo::audio {
namespace {

void add(Module63StopPlan& plan, Module63StopCall call,
         std::array<std::uint32_t, 7> args = {}, std::uint8_t count = 0) {
    plan.actions.push_back({call, args, count});
}

} // namespace

Module63StopPlan plan_module63_stop(const Module63StopInput& input) {
    Module63StopPlan plan{};
    add(plan, Module63StopCall::disable_capture_callback, {0x1d}, 1);
    add(plan, Module63StopCall::clear_capture_event, {0x1d}, 1);
    add(plan, Module63StopCall::stop_capture_event, {0x1d}, 1);

    plan.special_idle_path = input.state_field_2c == 1 &&
        input.state_field_10 == 5;
    if (plan.special_idle_path) {
        add(plan, Module63StopCall::configure_idle_mode, {3, 0}, 2);
        add(plan, Module63StopCall::configure_stream,
            {0, 2, 0, input.state_field_18, 0,
             input.state_byte_24, 0}, 7);
        return plan;
    }

    const auto code = module63_control_code(input.selector);
    if (!code) return plan;
    plan.selector_valid = true;
    plan.selected_control_code = *code;
    add(plan, Module63StopCall::send_control_code, {*code}, 1);
    return plan;
}

} // namespace echo::audio
