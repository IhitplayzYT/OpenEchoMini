#include "echo/recording/module28_markers.hpp"

namespace echo::recording {

std::uint16_t module28_counter_position(
    const std::uint32_t counter,
    const std::uint8_t state,
    const std::uint32_t scale_factor) noexcept {
    std::uint32_t position = 0;
    if (state > 1 && state != 3) {
        position = (scale_factor * (counter - 0x1db0U)) / 0x578U;
    } else {
        position = ((counter - 0x21fcU) * 200U) / 0x834U;
    }
    return static_cast<std::uint16_t>(position & 0xffffU);
}

Module28MarkerEventResult apply_module28_marker_event(
    Module28MarkerState& state,
    const std::uint16_t editing_mode,
    const std::uint16_t event_id) noexcept {
    constexpr std::uint16_t add_remove_event = 0xdf;
    constexpr std::uint16_t cancel_event = 0xe0;

    if (editing_mode == 1 && event_id == add_remove_event) {
        if (state.screen_index >= state.values.size()) {
            return {true, false};
        }
        state.values[state.screen_index] = static_cast<std::uint16_t>(state.counter_value);
        if (state.count < 0x1d) {
            ++state.count;
        }
        return {true, false};
    }

    if (editing_mode == 0 && event_id == add_remove_event) {
        if (state.screen_index >= state.values.size()) {
            return {true, false};
        }
        if (state.count == state.screen_index) {
            for (std::uint16_t index = 0; index < state.values.size(); ++index) {
                if (index == state.screen_index) {
                    continue;
                }
                if (state.values[index] != 0) {
                    state.active_marker_index = index;
                    state.counter_value = state.values[index];
                    break;
                }
                state.counter_value = 0x222e;
            }
            if (state.count != 0) {
                --state.count;
                state.values[state.screen_index] = 0;
                return {true, true};
            }
            state.values[state.screen_index] = 0;
            return {true, false};
        }
        if (state.screen_index < state.values.size()) {
            state.values[state.screen_index] = 0;
        }
        return {true, false};
    }

    if ((editing_mode == 0 || editing_mode == 1) && event_id == cancel_event) {
        return {true, false};
    }
    return {};
}

std::size_t compact_module28_marker_window(
    Module28MarkerState& state,
    const std::uint16_t minimum,
    const std::uint16_t maximum) noexcept {
    std::array<std::uint16_t, 30> retained{};
    std::size_t count = 0;
    for (const auto value : state.values) {
        if (minimum <= value && value <= maximum) {
            retained[count++] = value;
        }
    }
    state.values = retained;
    return count;
}

} // namespace echo::recording
