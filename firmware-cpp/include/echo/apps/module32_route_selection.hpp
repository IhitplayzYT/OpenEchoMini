#pragma once

#include <array>
#include <cstdint>

namespace echo::apps {

inline constexpr std::array<std::uint32_t, 6> module32_route_values{
    0x000006be, 0x00000372, 0x00000372,
    0x000006be, 0x000003b4, 0x000003b4,
};

struct Module32RoutePlan {
    bool changed{};
    bool selector_in_range{};
    bool clear_service_context{true};
    std::uint8_t selected_route{};
    std::uint32_t opaque_route_value{};
    bool start_service{};
    bool stop_service{};
    std::uint8_t return_value{};
};

// Models FUN_0307e278 for the observed selector table (indices 0 through 5).
[[nodiscard]] Module32RoutePlan plan_module32_route_selection(
    std::uint8_t current_route, std::uint8_t requested_route,
    bool service_active) noexcept;

} // namespace echo::apps
