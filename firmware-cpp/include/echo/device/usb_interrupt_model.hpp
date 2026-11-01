#pragma once

#include <cstdint>
#include <vector>

namespace echo::device {

enum class UsbTraceEvent {
    reset_to_suspend,
    suspend,
    warm_reset,
    resume,
};

struct UsbInterruptState {
    // Raw values observed in the firmware's state bytes. Their public API
    // meanings are intentionally left unnamed.
    std::uint8_t link_state{};
    bool suspend_transition_pending{};
};

struct UsbInterruptResult {
    UsbInterruptState state;
    std::vector<std::uint32_t> status_writes;
    std::vector<UsbTraceEvent> trace_events;
    bool reset_cleanup_requested{};
    bool endpoint_status_requested{};
};

class UsbInterruptModel {
public:
    // Models the observed status/mask interrupt handler. `pending_flags` is
    // status & enable_mask from the firmware. `additional_ack_mask` represents
    // the firmware's unknown DAT_030688e4 mask; callers must supply it.
    [[nodiscard]] static UsbInterruptResult process(
        std::uint32_t pending_flags,
        std::uint32_t additional_ack_mask,
        UsbInterruptState current_state);
};

} // namespace echo::device
