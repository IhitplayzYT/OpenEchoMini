#pragma once

#include <cstdint>
#include <vector>

namespace echo::apps {

enum class Module5NavigationEffect : std::uint8_t {
    call,
    event_guard_enter,
    event_guard_leave,
    notify_event,
    clear_event,
    write_context_u8,
    write_context_u16,
    write_shared_state_u32,
    set_browser_mode,
    set_navigation_flag,
    set_title_add_state,
    log_title_add,
    set_title_selection_flag,
    set_display_result,
    submit_display_result,
};

struct Module5NavigationAction {
    Module5NavigationEffect effect{};
    // Function address, event ID, or global/context offset as appropriate.
    std::uint32_t target{};
    std::uint32_t argument0{};
    std::uint32_t argument1{};
};

struct Module5NavigationContext {
    bool reset_requested{};
    bool event_196{};
    bool event_197{};
    bool event_198{};
    bool event_17a{};
    bool event_0b_query_nonzero{};
    std::uint8_t browser_mode{};
    std::uint8_t context_flag_6c{};
    std::uint8_t display_selector{};
    std::uint16_t state_code{};
    std::uint16_t title_index{};
    std::uint16_t context_value_1c4{};
    std::uint16_t context_value_2e{};
    // The original routine keeps parts of this local word unchanged on some
    // branches. Pass the observed caller-side starting value when available.
    std::uint32_t initial_display_result{};
};

struct Module5NavigationPlan {
    std::uint32_t display_result{};
    bool display_result_fully_known{};
    bool returns_minus_one{true};
    std::vector<Module5NavigationAction> actions;
};

// Models the confirmed event, mode, title-state, and display-result branches
// in module 5's update routine. Shared global records and external helpers are
// represented as effects; this function performs no UI or firmware I/O.
[[nodiscard]] Module5NavigationPlan plan_module5_navigation_update(
    const Module5NavigationContext& context);

} // namespace echo::apps
