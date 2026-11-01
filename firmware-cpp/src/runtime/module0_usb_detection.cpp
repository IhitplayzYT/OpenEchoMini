#include "echo/runtime/module0_usb_detection.hpp"

namespace echo::runtime {

Module0UsbDetectionResult advance_module0_usb_detection(
    Module0UsbDetectionState& state,
    std::uint32_t now_tick) noexcept {
    if (state.armed != 1) {
        return Module0UsbDetectionResult::pending;
    }

    if (state.detection_state == 1) {
        state.armed = 0;
        return Module0UsbDetectionResult::detected;
    }

    if (state.detection_state != 2 &&
        static_cast<std::uint32_t>(now_tick - state.last_timeout_tick) > 300U) {
        state.last_timeout_tick = now_tick;
        if (state.timeout_reported == 0) {
            state.timeout_reported = 1;
            return Module0UsbDetectionResult::first_timeout;
        }
    }

    return Module0UsbDetectionResult::pending;
}

} // namespace echo::runtime
