#pragma once

#include <cstdint>
#include <vector>

namespace echo::radio {

enum class Module60ActionKind : std::uint8_t {
    set_saved_frequency,
    apply_band_configuration,
    callback_slot,
    delay_ticks,
    signal_ui_event,
    clear_scan_state,
    configure_tuner,
    apply_slider_frequency,
    invoke_helper,
    configure_if_helper_returns_zero,
    load_mode_state,
    step_frequency_tenths,
    round_frequency_to_ten,
    select_preset,
    initialize_tuner_table,
    set_one_shot_flag,
};

struct Module60Action {
    Module60ActionKind kind{};
    std::int32_t value{};
    std::int32_t argument{};
    bool has_argument{};
};

struct Module60EventInput {
    std::uint32_t event_id{};
    std::int32_t event_parameter{};
    std::uint32_t saved_frequency{};
    std::uint8_t region_code{};
    std::uint8_t mode_byte{};
    std::uint8_t preset_direction{};
    std::uint8_t configuration_selector{};
    bool one_shot_flag_already_set{};
};

struct Module60EventPlan {
    std::uint32_t result{};
    std::vector<Module60Action> actions;
};

// Plans the observed event IDs in FUN_030945b6. Callback slots and UI event
// numbers are preserved as numeric values; the host does not invoke them.
[[nodiscard]] Module60EventPlan plan_module60_event(const Module60EventInput& input);

} // namespace echo::radio
