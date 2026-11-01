#pragma once

#include <cstdint>
#include <vector>

namespace echo::audio {

enum class Module63FinalizeAction : std::uint8_t {
    poll_external_status,
    service_wait_iteration,
    set_capture_mode,
    configure_capture_service,
    stop_recording_path,
};

struct Module63FinalizeInput {
    std::uint32_t service_state{};
    // Number of consecutive status==1 results before the first nonbusy result.
    // A value >= 2000 represents the observed loop limit.
    std::uint32_t busy_responses_before_clear{};
    std::uint16_t state_flags_0c{};
};

struct Module63FinalizePlan {
    std::vector<Module63FinalizeAction> actions;
    std::uint32_t poll_iterations{};
    std::uint32_t status_reads{};
    bool poll_limit_reached{};
    bool service_state_written{};
    std::uint32_t service_state_after{};
    bool flags_0c_written{};
    std::uint16_t flags_0c_after{};
    bool field_30_cleared{};
    bool field_34_cleared{};
    std::uint32_t return_value{1};
};

// Plans FUN_0308168e's bounded status poll and cleanup tail.
[[nodiscard]] Module63FinalizePlan plan_module63_finalize(
    const Module63FinalizeInput& input);

} // namespace echo::audio
