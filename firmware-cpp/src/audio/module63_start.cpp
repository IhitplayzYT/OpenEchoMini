#include "echo/audio/module63_start.hpp"

namespace echo::audio {
namespace {

void add(Module63StartPlan& plan, Module63StartCall call,
         std::array<std::uint32_t, 4> args = {}, std::uint8_t count = 0) {
    plan.actions.push_back({call, args, count});
}

} // namespace

Module63StartPlan plan_module63_start(const Module63StartInput& input) {
    Module63StartPlan plan{};
    if (input.service_state == 1) return plan;

    if (input.service_state == 2)
        add(plan, Module63StartCall::cleanup_mode_two);
    add(plan, Module63StartCall::arm_capture_callback);
    add(plan, Module63StartCall::acquire_capture_buffer, {0, 0, 0, 0}, 1);

    if (input.state_after_buffer_acquire == 1) return plan;

    plan.flags_0c_written = true;
    plan.flags_0c_after = static_cast<std::uint16_t>(input.state_flags_0c | 2u);
    plan.field_34_written = true;
    plan.field_34_after = 1;
    add(plan, Module63StartCall::submit_stream_five,
        {5, input.capture_buffer, input.state_field_44,
         input.state_field_3c >> 1}, 4);
    plan.status_written = true;
    plan.status_after = 1;
    plan.field_30_written = true;
    plan.field_30_after = 0;
    plan.flags_10_written = true;
    plan.flags_10_after = static_cast<std::uint16_t>(
        (input.state_flags_10 & 0xfffeu) | 2u);
    add(plan, Module63StartCall::set_capture_mode, {0, 2, 0, 0}, 2);

    if (input.state_field_2c == 0) {
        add(plan, Module63StartCall::notify_capture,
            {0, input.state_field_24, 0, 1}, 4);
        add(plan, Module63StartCall::apply_mode_command,
            {input.state_field_14, 0, 0, 0}, 1);
        add(plan, Module63StartCall::notify_device_state,
            {input.state_field_18, 0, 0, 0}, 1);
    }
    return plan;
}

} // namespace echo::audio
