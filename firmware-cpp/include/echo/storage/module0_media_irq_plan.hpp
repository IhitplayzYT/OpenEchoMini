#pragma once

#include <array>
#include <cstdint>

namespace echo::storage {

struct Module0MediaIrqPlan {
    bool valid_device_index{};
    std::uint8_t device_index{};
    std::uint32_t controller_base{};
    std::uint32_t status_register_offset{0x40};
    std::uint32_t status_ack_register_offset{0x44};
    std::uint32_t fifo_data_offset{0x200};
    bool receive_fifo_to_buffer{};
    std::uint8_t receive_words{};
    bool buffer_to_transmit_fifo{};
    std::uint8_t transmit_words{};
    std::uint32_t cursor_after{};
    std::uint8_t event_bits_to_set{};
    std::uint32_t controller_control_clear_mask{};
    std::array<std::uint16_t, 7> status_ack_order{};
    std::uint8_t status_ack_count{};
};

[[nodiscard]] inline Module0MediaIrqPlan plan_module0_media_irq(
    std::uint8_t device_index, std::uint32_t status,
    std::uint32_t cursor, std::uint32_t total_words) noexcept {
    Module0MediaIrqPlan plan{};
    plan.device_index = device_index;
    if (device_index > 1) return plan;
    plan.valid_device_index = true;
    plan.controller_base = device_index == 0 ? 0x60020000U : 0x60030000U;
    plan.cursor_after = cursor;

    const auto ack = [&plan](std::uint16_t bit) {
        plan.status_ack_order[plan.status_ack_count++] = bit;
    };
    if ((status & 0x20U) != 0) {
        plan.receive_fifo_to_buffer = true;
        plan.receive_words = 16;
        plan.cursor_after += 16;
        ack(0x20);
    }
    if ((status & 0x10U) != 0) {
        const auto remaining = total_words - plan.cursor_after;
        plan.transmit_words = static_cast<std::uint8_t>(
            remaining < 17U ? remaining : 16U);
        plan.cursor_after = remaining < 17U
            ? total_words : plan.cursor_after + 16U;
        plan.buffer_to_transmit_fifo = true;
        ack(0x10);
    }
    if ((status & 0x4U) != 0) {
        plan.event_bits_to_set |= 0x1U;
        ack(0x4);
    }
    if ((status & 0x8U) != 0) {
        plan.event_bits_to_set |= 0x2U;
        ack(0x8);
    }
    if ((status & 0x2000U) != 0) {
        plan.event_bits_to_set |= 0x2U;
        plan.controller_control_clear_mask = 0x2000U;
        ack(0x2000);
    }
    if ((status & 0x800U) != 0) ack(0x800);
    if ((status & 0x1000U) != 0) ack(0x1000);
    return plan;
}

} // namespace echo::storage
