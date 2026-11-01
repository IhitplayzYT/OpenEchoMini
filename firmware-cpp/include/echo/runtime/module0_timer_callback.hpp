#pragma once

#include <cstdint>

namespace echo::runtime {

// The first five words of the module 0 timer-list node.
struct Module0TimerCallbackNode {
    std::uint32_t next{};                   // +0x00
    std::uint32_t elapsed_ticks{};          // +0x04
    std::uint32_t period_ticks{};           // +0x08
    std::uint32_t remaining_invocations{};  // +0x0c; zero means repeat
    std::uint32_t callback_thumb_address{}; // +0x10
};

struct Module0TimerCallbackTick {
    Module0TimerCallbackNode node{};
    bool callback_due{};
    bool remove_node{};
};

// Models one node's visit in the module 0 tick traversal. It does not call the
// firmware callback or mutate the linked-list head.
[[nodiscard]] Module0TimerCallbackTick module0_advance_timer_callback(
    Module0TimerCallbackNode node) noexcept;

} // namespace echo::runtime
