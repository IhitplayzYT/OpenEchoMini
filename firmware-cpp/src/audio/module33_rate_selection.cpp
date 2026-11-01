#include "echo/audio/module33_rate_selection.hpp"

namespace echo::audio {

Module33RateSelectionPlan plan_module33_rate_selection(
    const std::uint32_t input_rate,
    const std::uint8_t mode) noexcept {
    Module33RateSelectionPlan plan{};
    plan.input_rate = input_rate;
    plan.processing_rate = input_rate;
    plan.mode = mode;

    switch (input_rate) {
    case 8000:
    case 11025:
    case 12000:
    case 16000:
    case 22050:
    case 24000:
        plan.processing_rate = 22050;
        break;
    case 32000:
    case 96000:
    case 128000:
    case 176400:
    case 192000:
        plan.processing_rate = 44100;
        break;
    default:
        break;
    }

    switch (mode) {
    case 1:
        plan.source = Module33CoefficientSource::table_a;
        plan.mode_recognized = true;
        break;
    case 2:
        plan.source = Module33CoefficientSource::table_b;
        plan.mode_recognized = true;
        break;
    case 3:
    case 4:
    case 5:
    case 8:
        plan.source = Module33CoefficientSource::table_a;
        plan.table_byte_offset = static_cast<std::uint16_t>((mode - 1) * 0x3e);
        plan.mode_recognized = true;
        break;
    case 6:
        plan.source = Module33CoefficientSource::table_a;
        plan.table_byte_offset = 0x3e;
        plan.mode_recognized = true;
        break;
    case 7:
        plan.source = Module33CoefficientSource::custom_state;
        plan.mode_recognized = true;
        for (std::size_t index = 0; index < plan.custom_writes.size(); ++index) {
            plan.custom_writes[index] = {
                static_cast<std::uint8_t>(3 + index * 2),
                static_cast<std::uint8_t>(0x0e + index * 0x0a),
                12,
            };
        }
        break;
    default:
        break;
    }
    return plan;
}

} // namespace echo::audio
