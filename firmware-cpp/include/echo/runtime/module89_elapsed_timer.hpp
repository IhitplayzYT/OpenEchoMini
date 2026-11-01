#pragma once

#include <array>
#include <cstdint>

namespace echo::runtime {

struct Module89DisplayTime {
    std::uint8_t hours{};
    std::uint8_t minutes{};
    std::uint8_t seconds{};
    std::uint8_t indicator{};
};

struct Module89ElapsedState {
    std::uint8_t mode{};
    std::uint8_t history_index{};
    std::uint8_t previous_history_index{};
    std::uint32_t poll_count{};
    std::uint32_t quantized_timer_tick{};
    Module89DisplayTime display{};
    std::array<Module89DisplayTime, 5> history{};
};

struct Module89TickEvents {
    bool time_refresh{};       // firmware event 0x1ae
    bool indicator_refresh{};  // firmware event 0x1af
};

// Advances FUN_0307b622 for one firmware poll using the external timer value
// sampled after its timer-service call. Returns false if the observed history
// index would write beyond the five-record buffer.
bool tick_module89_elapsed_timer(Module89ElapsedState& state,
                                 std::uint32_t timer_value,
                                 Module89TickEvents& events);

// Models the mode and five-record selection transitions observed in
// FUN_0307b4b4. Selecting a stored record copies it into the live display only
// while stopped.
void toggle_module89_running(Module89ElapsedState& state) noexcept;
bool select_module89_history(Module89ElapsedState& state, bool next) noexcept;

} // namespace echo::runtime
