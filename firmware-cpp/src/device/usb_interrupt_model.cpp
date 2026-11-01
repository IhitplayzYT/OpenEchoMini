#include "echo/device/usb_interrupt_model.hpp"

namespace echo::device {
namespace {

constexpr std::uint32_t kStatusEvent = 0x00000400;
constexpr std::uint32_t kSuspendEvent = 0x00000800;
constexpr std::uint32_t kWarmResetEvent = 0x00001000;
constexpr std::uint32_t kEndpointEvent = 0x00040000;
constexpr std::uint32_t kHighSpeedEvent = 0x40000000;
constexpr std::uint32_t kResumeEvent = 0x80000000;

} // namespace

UsbInterruptResult UsbInterruptModel::process(std::uint32_t pending_flags,
                                              std::uint32_t additional_ack_mask,
                                              UsbInterruptState current_state) {
    UsbInterruptResult result{current_state};
    const auto acknowledge = [&result](std::uint32_t value) {
        result.status_writes.push_back(value);
    };

    if ((pending_flags & kStatusEvent) != 0) {
        acknowledge(kStatusEvent);
    }
    if ((pending_flags & kSuspendEvent) != 0) {
        acknowledge(kSuspendEvent);
        if (result.state.link_state == 0) {
            result.state.link_state = 2;
            result.state.suspend_transition_pending = true;
            result.trace_events.push_back(UsbTraceEvent::suspend);
        } else {
            result.trace_events.push_back(UsbTraceEvent::reset_to_suspend);
            result.state.link_state = 0;
        }
    }
    if ((pending_flags & kWarmResetEvent) != 0) {
        result.trace_events.push_back(UsbTraceEvent::warm_reset);
        result.state.link_state = result.state.suspend_transition_pending ? 1 : 0;
        result.state.suspend_transition_pending = false;
        result.reset_cleanup_requested = true;
        acknowledge(kWarmResetEvent);
    }
    if ((pending_flags & kHighSpeedEvent) != 0) {
        acknowledge(kHighSpeedEvent);
    }
    if ((pending_flags & kResumeEvent) != 0) {
        result.trace_events.push_back(UsbTraceEvent::resume);
        result.state.link_state = 3;
        acknowledge(kResumeEvent);
    }
    if ((pending_flags & kEndpointEvent) != 0) {
        result.endpoint_status_requested = true;
    }

    const auto additional = pending_flags & additional_ack_mask;
    if (additional != 0) acknowledge(additional);
    return result;
}

} // namespace echo::device
