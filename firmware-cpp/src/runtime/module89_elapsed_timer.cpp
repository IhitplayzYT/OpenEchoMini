#include "echo/runtime/module89_elapsed_timer.hpp"

namespace echo::runtime {
namespace {
constexpr std::uint64_t kTicksToSecondsReciprocal = 0x51eb851full;

std::uint32_t quantize_timer(std::uint32_t timer_value) noexcept {
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(timer_value) *
         kTicksToSecondsReciprocal) >> 37);
}

bool indicator_due(std::uint32_t poll_count) noexcept {
    const auto quotient = static_cast<std::uint32_t>(
        (kTicksToSecondsReciprocal * poll_count) >> 36);
    return poll_count + quotient * std::uint32_t{0xffffffceu} == 0;
}

void advance_elapsed_time(Module89DisplayTime& display) noexcept {
    if (display.seconds < 59) {
        ++display.seconds;
        return;
    }
    display.seconds = 0;
    if (display.minutes < 59) {
        ++display.minutes;
        return;
    }
    display.minutes = 0;
    ++display.hours;
}
} // namespace

bool tick_module89_elapsed_timer(Module89ElapsedState& state,
                                 std::uint32_t timer_value,
                                 Module89TickEvents& events) {
    events = {};
    if (state.mode != 2) return true;
    if (state.history_index >= state.history.size()) return false;

    ++state.poll_count;
    const auto quantized = quantize_timer(timer_value);
    if (state.quantized_timer_tick != quantized) {
        state.quantized_timer_tick = quantized;
        advance_elapsed_time(state.display);
        events.time_refresh = true;
    }

    if (indicator_due(state.poll_count)) {
        const auto next = static_cast<std::uint8_t>(state.display.indicator + 3u);
        state.display.indicator = next >= 99 ? 0 : next;
        events.indicator_refresh = true;
    }

    state.history[state.history_index] = state.display;
    return true;
}

void toggle_module89_running(Module89ElapsedState& state) noexcept {
    state.mode = state.mode == 2 ? 0 : 2;
}

bool select_module89_history(Module89ElapsedState& state, bool next) noexcept {
    if (state.history_index >= state.history.size()) return false;
    state.previous_history_index = state.history_index;
    if (next) {
        state.history_index = static_cast<std::uint8_t>(
            (state.history_index + 1u) % state.history.size());
    } else {
        state.history_index = state.history_index == 0
                                  ? static_cast<std::uint8_t>(state.history.size() - 1u)
                                  : static_cast<std::uint8_t>(state.history_index - 1u);
    }
    if (state.mode != 2) state.display = state.history[state.history_index];
    return true;
}

} // namespace echo::runtime
