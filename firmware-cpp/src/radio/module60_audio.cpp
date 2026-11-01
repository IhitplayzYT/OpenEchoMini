#include "echo/radio/module60_audio.hpp"

namespace echo::radio {

Module60AudioPlan plan_module60_audio_setup(const Module60AudioInput& input) noexcept {
    Module60AudioPlan plan{};
    switch (input.sample_rate_selector) {
    case 0:
    case 2:
        plan.has_sample_rate = true;
        plan.sample_rate_hz = 32000;
        break;
    case 1:
    case 3:
        plan.has_sample_rate = true;
        plan.sample_rate_hz = 44100;
        break;
    case 4:
        plan.has_sample_rate = true;
        plan.sample_rate_hz = 48000;
        break;
    default:
        break;
    }

    if (input.sample_rate_selector < 2) {
        plan.device_format_code = 0x0f;
        plan.callback_buffer_size = input.small_buffer_variant == 0 ? 0x480 : 0x3f4;
    } else {
        plan.device_format_code = 0x17;
        plan.callback_buffer_size = 0xc00;
    }

    const auto toggle = input.dma_buffer_toggle == 0 ? 0u : 1u;
    plan.dma_starts = {{
        {5, input.stream5_handle,
         input.dma_buffer_base + toggle * 0x3000u,
         plan.callback_buffer_size, 0x03093fcbu},
        {4, input.stream4_handle,
         input.dma_buffer_base + (1u - toggle) * 0x3000u,
         plan.callback_buffer_size, 0x03093f89u},
    }};
    return plan;
}

} // namespace echo::radio
