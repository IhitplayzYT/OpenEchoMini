#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::storage {

struct Module0RegisterWrite {
    std::uint32_t address{};
    std::uint32_t value{};
};

struct Module0MediaHostInitPlan {
    bool valid_device_index{};
    std::uint8_t device_index{};
    std::uint8_t mode_parameter{};
    std::uint8_t context_mode{};
    std::uint32_t context_address{};
    std::uint32_t controller_base{};
    std::uint32_t controller_gate_address{0x40180084};
    std::uint32_t controller_gate_value{};
    std::uint32_t controller_gate_disable_value{};
    std::array<Module0RegisterWrite, 4> pinmux_writes{};
    std::uint8_t pinmux_write_count{};
    std::uint32_t reset_request{};
    std::uint32_t reset_ready_mask{0x7};
    std::uint16_t reset_poll_limit_ms{1000};
    std::uint16_t reset_timeout_code{0x1000};
    std::array<Module0RegisterWrite, 7> controller_setup_writes{};
    std::uint8_t controller_setup_write_count{};
    std::uint32_t ready_value{0x10};
};

// Captures the observed setup values for device indices 0 and 1. The caller
// must execute or replace these actions for its target; this function performs
// no MMIO, GPIO, lock, delay, or callback operation.
[[nodiscard]] inline Module0MediaHostInitPlan plan_module0_media_host_init(
    std::uint8_t device_index, std::uint8_t mode_parameter = 0) noexcept {
    Module0MediaHostInitPlan plan{};
    plan.device_index = device_index;
    plan.mode_parameter = mode_parameter;
    if (device_index > 1) return plan;

    plan.valid_device_index = true;
    plan.context_address = 0x0301d250U + device_index * 0x24U;
    plan.controller_base = device_index == 0 ? 0x60020000U : 0x60030000U;
    plan.controller_gate_value = device_index == 0 ? 0x01000000U : 0x02000000U;
    plan.controller_gate_disable_value = device_index == 0 ? 0x01000100U : 0x02000200U;
    plan.context_mode = mode_parameter == 0 ? 4 : 8;
    plan.reset_request = 7;

    if (device_index == 0) {
        plan.pinmux_writes = {{{0x40180030U, 0x009f009fU},
                               {0x50010020U, 0xfc005400U},
                               {0x50010024U, 0x003f0015U}, {}}};
        plan.pinmux_write_count = 3;
    } else {
        plan.pinmux_writes = {{{0x40180030U, 0x9f009f00U},
                               {0x50010000U, 0xffff5555U},
                               {0x50010004U, 0x003f0015U},
                               {0x50010024U, 0x00c00040U}}};
        plan.pinmux_write_count = 4;
    }

    plan.controller_setup_writes = {{{plan.controller_base + 0x4cU, 0x300f0010U},
                                      {plan.controller_base + 0x18U, 0},
                                      {plan.controller_base + 0x0cU, 0},
                                      {plan.controller_base + 0x64U, 600000U},
                                      {plan.controller_base + 0x14U, 0xffffff40U},
                                      {plan.controller_base + 0x44U, 0xffffffffU},
                                      {plan.controller_base + 0x24U, 0x283cU}}};
    plan.controller_setup_write_count = 7;
    return plan;
}

} // namespace echo::storage
