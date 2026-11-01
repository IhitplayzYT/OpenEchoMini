#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::recording {

struct Module28MarkerState {
    std::array<std::uint16_t, 30> values{};
    std::uint16_t count = 0;
    std::uint16_t screen_index = 0;
    std::uint16_t active_marker_index = 0;
    std::uint32_t counter_value = 0;
};

struct Module28MarkerEventResult {
    bool consumed = false;
    bool refresh_requested = false;
};

[[nodiscard]] std::uint16_t module28_counter_position(
    std::uint32_t counter,
    std::uint8_t state,
    std::uint32_t scale_factor) noexcept;

[[nodiscard]] Module28MarkerEventResult apply_module28_marker_event(
    Module28MarkerState& state,
    std::uint16_t editing_mode,
    std::uint16_t event_id) noexcept;

[[nodiscard]] std::size_t compact_module28_marker_window(
    Module28MarkerState& state,
    std::uint16_t minimum,
    std::uint16_t maximum) noexcept;

} // namespace echo::recording
