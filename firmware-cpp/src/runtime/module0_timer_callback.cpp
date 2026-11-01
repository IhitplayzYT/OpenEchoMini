#include "echo/runtime/module0_timer_callback.hpp"

namespace echo::runtime {

Module0TimerCallbackTick module0_advance_timer_callback(
    Module0TimerCallbackNode node) noexcept {
    Module0TimerCallbackTick result{node, false, false};
    if (node.callback_thumb_address == 0 || node.period_ticks == 0) {
        return result;
    }

    ++result.node.elapsed_ticks;
    if (result.node.elapsed_ticks < node.period_ticks) {
        return result;
    }

    result.callback_due = true;
    result.node.elapsed_ticks = 0;
    if (node.remaining_invocations != 0) {
        --result.node.remaining_invocations;
        result.remove_node = result.node.remaining_invocations == 0;
    }
    return result;
}

} // namespace echo::runtime
