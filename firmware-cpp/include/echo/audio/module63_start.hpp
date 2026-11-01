#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace echo::audio {

enum class Module63StartCall : std::uint8_t {
    cleanup_mode_two,
    arm_capture_callback,
    acquire_capture_buffer,
    submit_stream_five,
    set_capture_mode,
    notify_capture,
    apply_mode_command,
    notify_device_state,
};

struct Module63StartAction {
    Module63StartCall call{};
    std::array<std::uint32_t, 4> arguments{};
    std::uint8_t argument_count{};
};

struct Module63StartInput {
    std::uint32_t service_state{};
    std::uint32_t state_after_buffer_acquire{};
    std::uint32_t capture_buffer{};
    std::uint32_t state_field_24{};
    std::uint32_t state_field_2c{};
    std::uint32_t state_field_3c{};
    std::uint32_t state_field_44{};
    std::uint32_t state_field_14{};
    std::uint32_t state_field_18{};
    std::uint16_t state_flags_0c{};
    std::uint16_t state_flags_10{};
};

struct Module63StartPlan {
    std::vector<Module63StartAction> actions;
    bool flags_0c_written{};
    std::uint16_t flags_0c_after{};
    bool status_written{};
    std::uint32_t status_after{};
    bool field_34_written{};
    std::uint32_t field_34_after{};
    bool field_30_written{};
    std::uint32_t field_30_after{};
    bool flags_10_written{};
    std::uint16_t flags_10_after{};
    std::uint32_t return_value{1};
};

// Plans FUN_03081604's startup/first-transfer path; it performs no I/O.
[[nodiscard]] Module63StartPlan plan_module63_start(
    const Module63StartInput& input);

} // namespace echo::audio
