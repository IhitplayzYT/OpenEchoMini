#include "echo/audio/module63_control.hpp"

namespace echo::audio {
namespace {

constexpr std::array<std::uint8_t, 5> kControlCodes{6, 2, 4, 7, 8};

void add(Module63ModeCommandPlan& plan, Module63ControlCall call,
         std::uint32_t first, std::uint32_t state) {
    plan.actions.push_back({call, first, state});
}

} // namespace

std::optional<std::uint8_t> module63_control_code(
    std::uint32_t selector) noexcept {
    if (selector >= kControlCodes.size()) return std::nullopt;
    return kControlCodes[selector];
}

Module63ModeCommandPlan plan_module63_mode_command(
    const Module63ModeCommandInput& input) {
    Module63ModeCommandPlan plan{};
    switch (input.mode) {
    case 0: plan.resolved_mode_value = 4; break;
    case 1: plan.resolved_mode_value = 6; break;
    case 2: plan.resolved_mode_value = 8; break;
    default: plan.resolved_mode_value = input.fallback_mode_value; break;
    }

    const auto state = input.state_argument;
    switch (input.selector) {
    case 0:
        add(plan, Module63ControlCall::call_03061d3e, 6, state);
        add(plan, Module63ControlCall::call_03051a64, 2, state);
        add(plan, Module63ControlCall::call_03051a66,
            plan.resolved_mode_value, state);
        add(plan, Module63ControlCall::call_03051a62, 0, state);
        break;
    case 1:
        add(plan, Module63ControlCall::call_03061d3e, 2, state);
        break;
    case 2:
        add(plan, Module63ControlCall::call_03061d3e, 4, state);
        add(plan, Module63ControlCall::call_03051a66,
            plan.resolved_mode_value, state);
        add(plan, Module63ControlCall::call_03051a62, 0, state);
        break;
    case 3:
        add(plan, Module63ControlCall::call_03061d3e, 7, state);
        add(plan, Module63ControlCall::call_03051a64, 2, state);
        add(plan, Module63ControlCall::call_03051a66,
            plan.resolved_mode_value, state);
        add(plan, Module63ControlCall::call_03051a62, 0, state);
        break;
    case 4:
        add(plan, Module63ControlCall::call_03061d3e, 8, state);
        add(plan, Module63ControlCall::call_03051a64, 2, state);
        add(plan, Module63ControlCall::call_03051a66,
            plan.resolved_mode_value, state);
        add(plan, Module63ControlCall::call_03051a62, 0, state);
        break;
    default:
        break;
    }
    return plan;
}

} // namespace echo::audio
