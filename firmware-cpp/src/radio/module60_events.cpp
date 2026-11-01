#include "echo/radio/module60_events.hpp"

namespace echo::radio {
namespace {

void add_callback(Module60EventPlan& plan, const std::int32_t slot,
                  const std::int32_t argument, const bool has_argument = true) {
    plan.actions.push_back({Module60ActionKind::callback_slot, slot, argument, has_argument});
}

void configure_after_seek(Module60EventPlan& plan, const std::int32_t parameter) {
    plan.actions.push_back({Module60ActionKind::clear_scan_state, 0});
    plan.actions.push_back({Module60ActionKind::signal_ui_event, 0xa2});
    plan.actions.push_back({Module60ActionKind::configure_tuner, 0});
    if (parameter != 0) {
        add_callback(plan, 0x40, 0);
    }
}

} // namespace

Module60EventPlan plan_module60_event(const Module60EventInput& input) {
    Module60EventPlan plan{};
    const auto event = input.event_id;
    if (event == 0xab) {
        plan.result = 1;
        plan.actions.push_back({Module60ActionKind::set_saved_frequency,
                                static_cast<std::int32_t>(input.saved_frequency)});
        plan.actions.push_back({Module60ActionKind::apply_band_configuration,
                                input.region_code});
        add_callback(plan, 0x24, input.region_code);
        plan.actions.push_back({Module60ActionKind::delay_ticks, 100});
        add_callback(plan, 0x40, 1);
        plan.actions.push_back({Module60ActionKind::set_saved_frequency,
                                static_cast<std::int32_t>(input.saved_frequency)});
        plan.actions.push_back({Module60ActionKind::signal_ui_event, 0xb7});
        return plan;
    }
    if (event == 0xac) {
        plan.result = 1;
        configure_after_seek(plan, input.event_parameter);
        if (input.mode_byte == 1) {
            plan.actions.push_back({Module60ActionKind::signal_ui_event, 0x8a});
        }
        return plan;
    }
    if (event == 0xad) {
        plan.result = 1;
        plan.actions.push_back({Module60ActionKind::apply_slider_frequency, 0});
        return plan;
    }
    if (event == 0xae || event == 0xaf) {
        plan.result = 1;
        plan.actions.push_back({Module60ActionKind::invoke_helper,
                                event == 0xae ? 0x03094486 : 0x03094438});
        plan.actions.push_back({Module60ActionKind::configure_if_helper_returns_zero,
                                input.event_parameter});
        return plan;
    }
    if (event == 0xb0) {
        plan.result = 1;
        plan.actions = {{Module60ActionKind::load_mode_state, input.mode_byte}};
        add_callback(plan, 0x08, input.mode_byte != 1 ? 1 : 0);
        return plan;
    }
    if (event == 0xb1) {
        const auto step = input.preset_direction == 0 ? -10 : 10;
        plan.result = 1;
        plan.actions = {{Module60ActionKind::step_frequency_tenths, step},
                        {Module60ActionKind::round_frequency_to_ten, 0}};
        return plan;
    }
    if (event == 0xb2) {
        plan.result = 1;
        return plan;
    }
    if (event == 0xb3) {
        plan.result = 1;
        plan.actions.push_back({Module60ActionKind::select_preset,
                                input.preset_direction == 0 ? -1 : 1});
        plan.actions.push_back({Module60ActionKind::signal_ui_event, 0x8b});
        configure_after_seek(plan, 1);
        if (input.mode_byte == 1) {
            plan.actions.push_back({Module60ActionKind::signal_ui_event, 0x8a});
        }
        return plan;
    }
    if (event == 0xb4) {
        plan.result = 1;
        plan.actions.push_back({Module60ActionKind::signal_ui_event, 0x8a});
        return plan;
    }
    if (event == 0xb5) {
        plan.result = 1;
        plan.actions = {{Module60ActionKind::initialize_tuner_table, 0}};
        add_callback(plan, 0x1c, 1);
        add_callback(plan, 0x10, 0, false);
        return plan;
    }
    if (event == 0xb6) {
        plan.result = 1;
        plan.actions.push_back({Module60ActionKind::initialize_tuner_table,
                                input.configuration_selector});
        if (!input.one_shot_flag_already_set) {
            plan.actions.push_back({Module60ActionKind::set_one_shot_flag, 1});
        }
        return plan;
    }
    return plan;
}

} // namespace echo::radio
