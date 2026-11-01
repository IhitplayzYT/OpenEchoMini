#include "echo/audio/module63_wait.hpp"

namespace echo::audio {

Module63WaitPlan plan_module63_wait(const Module63WaitInput& input) {
    Module63WaitPlan plan{};
    plan.actions.push_back({Module63WaitCall::configure_wait_event,
                            {0x67, 0, 2}, 1});
    plan.actions.push_back({Module63WaitCall::configure_wait_flag,
                            {0x68, 0, 2}, 1});
    if (!input.flag_ready_on_entry) {
        // The routine decrements the signed register value and breaks only
        // when it becomes zero. A zero initial value wraps through 2^32-1.
        const std::uint64_t limit = input.initial_poll_budget == 0
            ? (std::uint64_t{1} << 32)
            : input.initial_poll_budget;
        const auto needed = input.iterations_until_ready == 0
            ? std::uint64_t{1} : input.iterations_until_ready;
        plan.poll_iterations = needed < limit ? needed : limit;
        plan.poll_limit_reached = needed >= limit;
        if (plan.poll_iterations != 0) {
            plan.actions.push_back({Module63WaitCall::service_wait, {},
                                    plan.poll_iterations});
            plan.actions.push_back({Module63WaitCall::service_delay, {},
                                    plan.poll_iterations});
            plan.actions.push_back({Module63WaitCall::service_tick, {1, 0, 0},
                                    plan.poll_iterations});
        }
    }
    plan.actions.push_back({Module63WaitCall::clear_wait_flag, {}, 1});
    return plan;
}

} // namespace echo::audio
