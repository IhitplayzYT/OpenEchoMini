#include "echo/apps/module27_playback_screen.hpp"

#include "echo/runtime/event_flags.hpp"

#include <array>

namespace echo::apps {

Module27PlaybackRoute route_module27_playback_screen(
    bool event_28_set, bool event_1ce_set, bool event_1_set,
    std::uint8_t state_byte_35e) noexcept {
    if (event_28_set) return Module27PlaybackRoute::suppressed_by_event_28;
    if (event_1ce_set) return Module27PlaybackRoute::suppressed_by_event_1ce;
    if (event_1_set) return Module27PlaybackRoute::alternate_event_path;
    return state_byte_35e == 0
               ? Module27PlaybackRoute::first_screen_handler
               : Module27PlaybackRoute::second_screen_handler;
}

Module27PlaybackEventAction classify_module27_playback_event(
    std::uint8_t event_id, Module27PlaybackRoute route) noexcept {
    if (event_id == 0x4f) {
        return Module27PlaybackEventAction::scan_timestamp_records;
    }
    if (event_id != 0x46) return Module27PlaybackEventAction::unmodeled;
    if (route == Module27PlaybackRoute::first_screen_handler) {
        return Module27PlaybackEventAction::metadata_formatter_d3d0;
    }
    if (route == Module27PlaybackRoute::second_screen_handler) {
        return Module27PlaybackEventAction::metadata_formatter_c674;
    }
    return Module27PlaybackEventAction::unmodeled;
}

bool consume_module27_event_3a(echo::runtime::EventFlagBank& events,
                               bool first_screen_handler) noexcept {
    if (!events.test_and_clear(0x3a)) return false;

    constexpr std::array<std::uint32_t, 10> raised{
        0x42, 0x4b, 0x46, 0x4c, 0x3f, 0x40, 0x48, 0x49, 0x4e, 0x3b};
    for (const auto event_id : raised) (void)events.set(event_id);
    (void)events.clear(0x4a);
    (void)events.set(0x21);

    if (first_screen_handler) {
        (void)events.clear(0xae);
        (void)events.clear(0xaf);
        (void)events.clear(0xb0);
    }
    return true;
}

void initialize_module27_startup_event_state(
    echo::runtime::EventFlagBank& events, Module27ControlTimerState& timers,
    std::uint32_t now_tick) noexcept {
    (void)events.set(0x39);
    constexpr std::array<std::uint32_t, 8> cleared{
        0x53, 0x55, 0x17c, 0x1da, 0x82, 0x1b6, 0x1b5, 0x11d};
    for (const auto event_id : cleared) (void)events.clear(event_id);
    timers.watchdog_interval = 10;
    timers.watchdog_start_tick = now_tick;
}

Module27ControlTimerResult advance_module27_control_timers(
    echo::runtime::EventFlagBank& events, std::uint8_t control_state,
    std::uint32_t now_tick, Module27ControlTimerState& timers) noexcept {
    Module27ControlTimerResult result{};
    if (control_state != 0 && control_state != 3) return result;

    if (static_cast<std::uint32_t>(now_tick - timers.last_refresh_tick) > 0x28) {
        timers.last_refresh_tick = now_tick;
        (void)events.set(0x3f);
        result.refresh_event_raised = true;
    }

    const auto watchdog_elapsed =
        static_cast<std::uint32_t>(now_tick - timers.watchdog_start_tick);
    if (control_state == 0 && watchdog_elapsed > timers.watchdog_interval) {
        timers.watchdog_interval = 10;
        timers.watchdog_start_tick = now_tick;
        (void)events.set(0x1b0);
        result.timeout_event_raised = true;
    }
    return result;
}

Module27SelectionEventResult consume_module27_event_4e(
    echo::runtime::EventFlagBank& events, bool first_screen_handler,
    bool record_lookup_succeeded, bool selected_index_changed,
    bool service_ready, bool playback_context_is_mode1) noexcept {
    if (!events.test_and_clear(0x4e)) return {};

    const bool reset_buffers = first_screen_handler &&
                               record_lookup_succeeded &&
                               selected_index_changed;
    if (reset_buffers) {
        (void)events.clear(0xae);
        (void)events.clear(0xaf);
        (void)events.clear(0xb0);
    }

    const bool accepted = record_lookup_succeeded && service_ready &&
                         !playback_context_is_mode1;
    if (!accepted) {
        (void)events.set(0x1b0);
        return {true, false, reset_buffers};
    }

    (void)events.set(0x4f);
    if (first_screen_handler) {
        (void)events.clear(0x46);
    } else {
        (void)events.set(0x46);
    }
    return {true, true, reset_buffers};
}

} // namespace echo::apps
