#include "echo/apps/module32_route_selection.hpp"

namespace echo::apps {

Module32RoutePlan plan_module32_route_selection(
    const std::uint8_t current_route, const std::uint8_t requested_route,
    const bool service_active) noexcept {
    const bool in_range = requested_route < module32_route_values.size();
    Module32RoutePlan plan{};
    plan.changed = current_route != requested_route;
    plan.selector_in_range = in_range;
    plan.selected_route = plan.changed ? requested_route : current_route;
    plan.return_value = 0;

    if (!plan.changed) return plan;
    if (requested_route == 0) {
        plan.stop_service = service_active;
    } else if (in_range) {
        plan.opaque_route_value = module32_route_values[requested_route];
        plan.start_service = !service_active;
    }
    return plan;
}

} // namespace echo::apps
