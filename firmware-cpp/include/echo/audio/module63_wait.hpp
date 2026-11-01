#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace echo::audio {

enum class Module63WaitCall : std::uint8_t {
    configure_wait_event,
    configure_wait_flag,
    service_wait,
    service_delay,
    service_tick,
    clear_wait_flag,
};

struct Module63WaitAction {
    Module63WaitCall call{};
    std::array<std::uint32_t, 3> arguments{};
    std::uint64_t repeat_count{1};
};

struct Module63WaitInput {
    bool flag_ready_on_entry{};
    // Number of polling iterations until the flag becomes ready. This is
    // supplied separately from the firmware's state-derived poll budget.
    std::uint64_t iterations_until_ready{};
    std::uint32_t initial_poll_budget{};
};

struct Module63WaitPlan {
    std::vector<Module63WaitAction> actions;
    std::uint64_t poll_iterations{};
    bool poll_limit_reached{};
    bool wait_flag_cleared{true};
};

// Plans FUN_03080d58's event setup, bounded polling and flag reset.
[[nodiscard]] Module63WaitPlan plan_module63_wait(
    const Module63WaitInput& input);

} // namespace echo::audio
