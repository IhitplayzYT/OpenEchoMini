#include "echo/audio/module33_filter_setup.hpp"

namespace echo::audio {
namespace {

constexpr std::uint32_t kStatePointer = 0x0308b26c;
constexpr std::uint32_t kContextPointer = 0x0308b268;
constexpr std::uint32_t kApplyUiState = 0x03069628;
constexpr std::uint32_t kNotifyMode = 0x03068e80;
constexpr std::uint32_t kNotifyPriorState = 0x03068ecc;
constexpr std::uint32_t kConfigureRateMode = 0x0308aebc;
constexpr std::uint32_t kNotifyService = 0x03068fdc;
constexpr std::uint32_t kCleanup = 0x03068402;

void push(Module33FilterSetupPlan& plan,
          const Module33FilterSetupEffect effect,
          const std::uint32_t target,
          const std::uint32_t value = 0,
          const std::uint32_t auxiliary = 0) noexcept {
    if (plan.step_count < plan.steps.size()) {
        plan.steps[plan.step_count++] = {effect, target, value, auxiliary};
    }
}

} // namespace

Module33FilterSetupPlan plan_module33_filter_setup(
    const Module33FilterSetupContext& context) noexcept {
    Module33FilterSetupPlan plan{};

    if (context.mode == 0) {
        if (context.previous_state >= 1 && context.previous_state <= 3) {
            push(plan, Module33FilterSetupEffect::notify, kNotifyPriorState,
                 0x16 + context.previous_state);
            push(plan, Module33FilterSetupEffect::write_shared_state,
                 kStatePointer, 0);
        }
    } else if (context.mode <= 7) {
        plan.configures_filter = true;
        push(plan, Module33FilterSetupEffect::call, kApplyUiState, 0x25, 7);

        if (context.shifted_shared_counter <= 0x480 &&
            context.sample_rate <= 48000) {
            plan.selected_state = 1;
        } else if (context.shifted_shared_counter < 0x1000 &&
                   (context.sample_rate <= 48000 ||
                    (context.shifted_shared_counter <= 0x480 &&
                     context.sample_rate <= 88200))) {
            plan.selected_state = 2;
        } else {
            plan.selected_state = 3;
        }

        const auto event_id = 0x16 + plan.selected_state;
        push(plan, Module33FilterSetupEffect::notify, kNotifyMode, event_id);
        push(plan, Module33FilterSetupEffect::write_shared_state,
             kStatePointer, plan.selected_state);
        push(plan, Module33FilterSetupEffect::configure_rate_mode,
             kConfigureRateMode, context.mode, context.sample_rate);
        push(plan, Module33FilterSetupEffect::notify, kNotifyService, 0x104);
    }

    push(plan, Module33FilterSetupEffect::call, kCleanup);
    push(plan, Module33FilterSetupEffect::clear_context_field,
         kContextPointer, 0x98);
    return plan;
}

} // namespace echo::audio
