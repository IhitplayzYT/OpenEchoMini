#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::audio {

enum class Module33FilterSetupEffect : std::uint8_t {
    call,
    notify,
    write_shared_state,
    configure_rate_mode,
    clear_context_field,
};

struct Module33FilterSetupStep {
    Module33FilterSetupEffect effect{};
    // Direct helper address, or the observed global-pointer address.
    std::uint32_t target{};
    std::uint32_t value{};
    std::uint32_t auxiliary{};
};

struct Module33FilterSetupContext {
    std::uint8_t mode{};
    std::uint32_t previous_state{};
    std::uint32_t shifted_shared_counter{};
    std::uint32_t sample_rate{};
};

struct Module33FilterSetupPlan {
    bool configures_filter{};
    std::uint32_t selected_state{};
    std::array<Module33FilterSetupStep, 8> steps{};
    std::size_t step_count{};
};

// Plans the ordered effects observed in FUN_03087ee4 around module 33's
// rate/mode selector. Runtime event, shared-memory, and cleanup helpers stay
// outside this portable model.
[[nodiscard]] Module33FilterSetupPlan plan_module33_filter_setup(
    const Module33FilterSetupContext& context) noexcept;

} // namespace echo::audio
