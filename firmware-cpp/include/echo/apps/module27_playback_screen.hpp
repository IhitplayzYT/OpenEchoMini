#pragma once

#include <cstdint>
#include <array>

namespace echo::runtime { class EventFlagBank; }

namespace echo::apps {

enum class Module27PlaybackRoute {
    suppressed_by_event_28,
    suppressed_by_event_1ce,
    first_screen_handler,
    second_screen_handler,
    alternate_event_path,
};

enum class Module27PlaybackEventAction {
    unmodeled,
    metadata_formatter_d3d0,
    metadata_formatter_c674,
    scan_timestamp_records,
};

struct Module27SelectionEventResult {
    bool consumed{};
    bool accepted{};
    bool reset_text_buffers{};
};

struct Module27ControlTimerState {
    std::uint32_t last_refresh_tick{};
    std::uint32_t watchdog_start_tick{};
    std::uint32_t watchdog_interval{};
};

struct Module27ControlTimerResult {
    bool refresh_event_raised{};
    bool timeout_event_raised{};
};

// Applies the confirmed startup event mutations and watchdog initialization
// from module 27's Thumb entry. The other global/service initialization calls
// in that entry remain outside this model.
void initialize_module27_startup_event_state(
    echo::runtime::EventFlagBank& events, Module27ControlTimerState& timers,
    std::uint32_t now_tick) noexcept;

struct Module27TextRegion {
    std::uint8_t event_id;
    std::uint16_t x_left;
    std::uint16_t y_top;
    std::uint16_t x_right;
    std::uint16_t y_bottom;
    std::uint16_t scroll_threshold;
};

inline constexpr std::array<Module27TextRegion, 3> module27_text_regions{{
    {0xae, 0x84, 0x32, 0x8f, 0x76, 0x44},
    {0xaf, 0x84, 0x86, 0x8f, 0xca, 0x44},
    {0xb0, 0x84, 0xdb, 0x8f, 0x11f, 0x44},
}};

// The firmware clears a region's event below the measured extent threshold
// and sets it at or above the threshold. The measurement unit is supplied by
// its shared text helper and is not recovered here.
[[nodiscard]] constexpr bool module27_text_needs_scroll(
    const Module27TextRegion& region, std::uint32_t measured_extent) noexcept {
    return measured_extent >= region.scroll_threshold;
}

// Models only FUN_0307e9fe's dispatch gate. Event flags are supplied by the
// caller; state_byte_35e selects FUN_0307d894 (zero) or FUN_0307e2a8 (nonzero)
// when event 1 is clear. Screen rendering and event mutation are caller-owned.
[[nodiscard]] Module27PlaybackRoute route_module27_playback_screen(
    bool event_28_set, bool event_1ce_set, bool event_1_set,
    std::uint8_t state_byte_35e) noexcept;

// Identifies the two event effects directly connected to recovered helpers:
// event 0x46 refreshes playback metadata using the handler-specific formatter;
// event 0x4f rescans timestamp records through FUN_0307b6cc.
[[nodiscard]] Module27PlaybackEventAction classify_module27_playback_event(
    std::uint8_t event_id, Module27PlaybackRoute route) noexcept;

// Consumes event 0x3a and reproduces its observed event-bit fan-out. The first
// handler also clears the three display-state events 0xae, 0xaf, and 0xb0.
[[nodiscard]] bool consume_module27_event_3a(
    echo::runtime::EventFlagBank& events, bool first_screen_handler) noexcept;

// Models the periodic refresh and state-0 timeout tail of FUN_0307c128.
// Tick units and the meaning of the surrounding control state are supplied
// by the caller; all elapsed comparisons use the firmware's unsigned wrap.
[[nodiscard]] Module27ControlTimerResult advance_module27_control_timers(
    echo::runtime::EventFlagBank& events, std::uint8_t control_state,
    std::uint32_t now_tick, Module27ControlTimerState& timers) noexcept;

// Models the event-0x4e path after its shared record and service checks have
// been evaluated by the caller. A true record_lookup_succeeded corresponds
// to FUN_0307bc28 returning zero.
[[nodiscard]] Module27SelectionEventResult consume_module27_event_4e(
    echo::runtime::EventFlagBank& events, bool first_screen_handler,
    bool record_lookup_succeeded, bool selected_index_changed,
    bool service_ready, bool playback_context_is_mode1) noexcept;

} // namespace echo::apps
