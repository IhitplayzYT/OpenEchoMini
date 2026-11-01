#include "echo/runtime/module0_dma.hpp"

namespace echo::runtime {

Module0DmaPlan module0_plan_dma_transfer(
    const std::uint8_t requested_channel, const std::uint8_t busy_channels,
    const std::uint32_t transfer_bytes) noexcept {
    Module0DmaPlan plan{};
    if (requested_channel >= Module0DmaPlan::channel_count) {
        plan.error = Module0DmaPlanError::invalid_requested_channel;
        return plan;
    }

    auto selected = requested_channel;
    const auto is_busy = [busy_channels](const std::uint8_t channel) noexcept {
        return (busy_channels & static_cast<std::uint8_t>(1U << channel)) != 0;
    };
    if (is_busy(selected)) {
        selected = 0xffU;
        for (std::uint8_t channel = 0;
             channel < Module0DmaPlan::channel_count - 1; ++channel) {
            if (!is_busy(channel)) {
                selected = channel;
                break;
            }
        }
    }
    if (selected == 0xffU) {
        plan.error = Module0DmaPlanError::no_available_channel;
        return plan;
    }

    const auto chunks = transfer_bytes <= Module0DmaPlan::chunk_bytes
        ? 1U
        : 1U + (transfer_bytes - 1U) / Module0DmaPlan::chunk_bytes;
    if (chunks > Module0DmaPlan::maximum_chunks) {
        plan.error = Module0DmaPlanError::transfer_too_large;
        return plan;
    }

    plan.channel = selected;
    plan.chunk_count = static_cast<std::uint8_t>(chunks);
    for (std::uint32_t i = 0; i < chunks; ++i) {
        const auto offset = i * Module0DmaPlan::chunk_bytes;
        const auto remaining = transfer_bytes - offset;
        plan.chunks[i] = {
            offset,
            remaining < Module0DmaPlan::chunk_bytes
                ? remaining
                : Module0DmaPlan::chunk_bytes,
        };
    }
    return plan;
}

} // namespace echo::runtime
