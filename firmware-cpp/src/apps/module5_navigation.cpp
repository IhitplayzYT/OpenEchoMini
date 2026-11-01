#include "echo/apps/module5_navigation.hpp"

namespace echo::apps {
namespace {

constexpr std::uint32_t kResetRoutine = 0x0307b542;
constexpr std::uint32_t kModeSet = 0x0305870a;
constexpr std::uint32_t kModeRefresh = 0x0307b8b4;
constexpr std::uint32_t kDisplaySubmit = 0x03069206;

void append(Module5NavigationPlan& plan,
            const Module5NavigationEffect effect,
            const std::uint32_t target = 0,
            const std::uint32_t argument0 = 0,
            const std::uint32_t argument1 = 0) {
    plan.actions.push_back({effect, target, argument0, argument1});
}

void call(Module5NavigationPlan& plan, const std::uint32_t address,
          const std::uint32_t argument0 = 0,
          const std::uint32_t argument1 = 0) {
    append(plan, Module5NavigationEffect::call, address, argument0, argument1);
}

std::uint8_t apply_mode_event(Module5NavigationPlan& plan,
                              const std::uint8_t current_mode,
                              const std::uint8_t first_mode,
                              const std::uint8_t second_mode,
                              const std::uint32_t first_state) {
    if (current_mode == first_mode) {
        call(plan, kModeRefresh, first_state);
        return current_mode;
    }
    append(plan, Module5NavigationEffect::set_browser_mode, 0x0e, first_mode);
    call(plan, kModeSet, first_mode);
    call(plan, kModeRefresh, first_state);
    append(plan, Module5NavigationEffect::set_browser_mode, 0x0e, second_mode);
    call(plan, kModeSet, second_mode);
    return second_mode;
}

} // namespace

Module5NavigationPlan plan_module5_navigation_update(
    const Module5NavigationContext& context) {
    Module5NavigationPlan plan{};
    plan.display_result = context.initial_display_result;

    append(plan, Module5NavigationEffect::event_guard_enter, 0x23);
    if (context.reset_requested) {
        append(plan, Module5NavigationEffect::write_context_u16, 0x42, 0);
        append(plan, Module5NavigationEffect::write_context_u8, 0x6c, 0);
        call(plan, 0x0306e452);
        call(plan, 0x0306e50c);
        append(plan, Module5NavigationEffect::write_context_u8, 0x217, 0);
        call(plan, kResetRoutine);
        append(plan, Module5NavigationEffect::notify_event, 0x196);
    }

    if (context.event_196 || context.event_197 || context.event_198) {
        call(plan, 0x03069628, 7);
    }
    if (context.event_0b_query_nonzero) {
        call(plan, 0x0306901e);
    } else {
        append(plan, Module5NavigationEffect::notify_event, 0x198);
    }

    auto browser_mode = context.browser_mode;
    if (context.event_196) {
        browser_mode = apply_mode_event(plan, browser_mode, 2, 4, 2);
    }
    if (context.event_198) {
        browser_mode = apply_mode_event(plan, browser_mode, 4, 2, 4);
    }
    append(plan, Module5NavigationEffect::event_guard_leave, 0x23);

    switch (context.state_code) {
    case 0:
        plan.display_result &= 0xffffff00U;
        plan.display_result_fully_known = false;
        break;
    case 1:
        plan.display_result &= 0xffff0000U;
        if (context.context_flag_6c == 1) {
            plan.display_result = (plan.display_result & 0xffff0000U) | 0x0bU;
        }
        plan.display_result_fully_known = false;
        break;
    case 15:
        plan.display_result = (plan.display_result & 0xffff0000U) | 6U;
        plan.display_result_fully_known = false;
        append(plan, Module5NavigationEffect::set_display_result,
               plan.display_result);
        call(plan, kDisplaySubmit, 10, 0);
        return plan;
    case 9:
        append(plan, Module5NavigationEffect::set_title_add_state, 3);
        append(plan, Module5NavigationEffect::log_title_add,
               context.title_index);
        if (context.title_index == 0x4b) {
            plan.display_result = 1;
            append(plan, Module5NavigationEffect::set_title_selection_flag, 0);
            plan.display_result_fully_known = true;
        } else if (context.title_index == 0x0b) {
            plan.display_result = 3;
            append(plan, Module5NavigationEffect::set_title_selection_flag, 1);
            plan.display_result_fully_known = true;
        } else if (context.title_index == 3) {
            plan.display_result = 6;
            append(plan, Module5NavigationEffect::set_title_selection_flag, 1);
            plan.display_result_fully_known = true;
        } else {
            plan.display_result_fully_known = false;
        }
        append(plan, Module5NavigationEffect::notify_event, 0x13d);
        break;
    case 5:
        plan.display_result = 0x00010000U | context.context_value_1c4;
        plan.display_result_fully_known = true;
        append(plan, Module5NavigationEffect::notify_event, 0x0a8);
        break;
    default:
        plan.display_result_fully_known = false;
        break;
    }

    if (context.event_17a) {
        append(plan, Module5NavigationEffect::set_navigation_flag, 1);
        plan.display_result = 0x005a0000U;
        plan.display_result_fully_known = true;
        if (context.context_value_2e == 0) {
            append(plan, Module5NavigationEffect::clear_event, 0x17a);
            plan.display_result &= 0xffffff00U;
            append(plan, Module5NavigationEffect::write_shared_state_u32, 4, 0);
        }
    }

    append(plan, Module5NavigationEffect::set_display_result,
           plan.display_result);
    call(plan, kDisplaySubmit, context.display_selector, 0);
    return plan;
}

} // namespace echo::apps
