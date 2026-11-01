#pragma once

#include <cstdint>

namespace echo::runtime {

struct Module0UsbDetectionState {
    std::uint8_t detection_state = 0;
    std::uint8_t armed = 0;
    std::uint8_t timeout_reported = 0;
    std::uint32_t last_timeout_tick = 0;
};

enum class Module0UsbDetectionResult {
    pending,
    detected,
    first_timeout,
};

[[nodiscard]] Module0UsbDetectionResult advance_module0_usb_detection(
    Module0UsbDetectionState& state,
    std::uint32_t now_tick) noexcept;

} // namespace echo::runtime
