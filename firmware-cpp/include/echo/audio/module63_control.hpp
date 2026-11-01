#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace echo::audio {

// FUN_0308057c accepts selectors 0..4 and forwards these control codes.
[[nodiscard]] std::optional<std::uint8_t> module63_control_code(
    std::uint32_t selector) noexcept;

enum class Module63ControlCall : std::uint8_t {
    call_03061d3e,
    call_03051a64,
    call_03051a66,
    call_03051a62,
};

struct Module63ControlAction {
    Module63ControlCall call{};
    std::uint32_t first_argument{};
    std::uint32_t state_argument{};
};

struct Module63ModeCommandInput {
    std::uint32_t selector{};
    std::uint8_t mode{};
    std::uint32_t state_argument{};
    // In the observed ABI, an unrecognized mode preserves entry r4.
    std::uint32_t fallback_mode_value{};
};

struct Module63ModeCommandPlan {
    std::uint32_t resolved_mode_value{};
    std::vector<Module63ControlAction> actions;
};

// Plans calls made by FUN_030805aa without invoking external firmware code.
[[nodiscard]] Module63ModeCommandPlan plan_module63_mode_command(
    const Module63ModeCommandInput& input);

} // namespace echo::audio
