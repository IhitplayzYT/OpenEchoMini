#include "echo/transport/module51_dispatch.hpp"

namespace echo::transport {
namespace {

void add_wait(Module51Plan& plan, const std::uint32_t event,
              const std::uint32_t limit, const bool output_context = false) {
    plan.actions.push_back({Module51ActionKind::clear_completion});
    plan.actions.push_back({Module51ActionKind::signal_event, event});
    plan.actions.push_back({Module51ActionKind::submit_request, 0, 0, output_context});
    plan.actions.push_back({Module51ActionKind::bounded_wait, 0, limit, false, 3});
    plan.actions.push_back({Module51ActionKind::clear_completion});
}

} // namespace

Module51Plan plan_module51_command(const Module51Input& in) {
    Module51Plan plan{};
    const auto command = in.command;
    if (command >= 3 && command <= 7) {
        constexpr std::array<std::uint8_t, 5> word_index{2, 4, 5, 1, 0};
        plan.result = 1;
        plan.has_value_output = true;
        plan.value_output = in.state_words[word_index[command - 3]];
        return plan;
    }
    if (command == 0x11) {
        plan.result = 1;
        plan.has_value_output = true;
        plan.value_output = in.state_words[3];
        return plan;
    }
    if (command == 9 || command == 0x0b || command == 0x0d || command == 0x10) {
        return plan;
    }
    if (command == 0x0c) {
        plan.result = 1;
        plan.actions = {{Module51ActionKind::signal_event, 4},
                        {Module51ActionKind::submit_request}};
        return plan;
    }
    if (command == 8) {
        add_wait(plan, 1, in.command8_wait_limit);
        plan.result = in.command8_secondary_flag ? 0u : 1u;
        return plan;
    }
    if (command == 0x0e || command == 0x0f) {
        const auto limit = command == 0x0e ? in.command14_wait_limit
                                           : in.command15_wait_limit;
        add_wait(plan, command == 0x0e ? 0x0b : 0x0d, limit, command == 0x0e);
        plan.result = 1;
        return plan;
    }
    if (command != 10) return plan;

    const auto& state = in.state_words;
    if (state[9] != 1) {
        if (state[10] == 0) {
            plan.state10_written = true;
            plan.state10_after = 1;
            plan.actions = {{Module51ActionKind::signal_event, 4},
                            {Module51ActionKind::submit_request}};
            plan.result = 2;
        } else if (state[10] == 1) {
            plan.result = 2;
        }
        return plan;
    }
    if (state[11] == 1) return plan;

    const auto second_buffer = in.buffer_toggle != 0;
    plan.result = 1;
    plan.has_count_output = true;
    plan.count_output = state[8];
    plan.state9_written = true;
    plan.state9_after = 0;
    plan.state10_written = true;
    plan.state10_after = 1;
    plan.toggle_written = true;
    plan.toggle_after = second_buffer ? 0 : 1;
    plan.has_transfer = true;
    plan.transfer.buffer_handle = second_buffer ? in.secondary_buffer : in.primary_buffer;
    const auto descriptor_offset = second_buffer ? 4u : 0u;
    for (std::size_t i = 0; i < plan.transfer.descriptor.size(); ++i) {
        plan.transfer.descriptor[i] = in.descriptors[descriptor_offset + i];
    }
    plan.transfer.byte_count = (state[3] * state[8]) >> 4;
    plan.transfer.toggle_after = plan.toggle_after;
    plan.transfer_wait_has_no_timeout = true;
    plan.actions = {{Module51ActionKind::signal_event, 4},
                    {Module51ActionKind::submit_request},
                    {Module51ActionKind::clear_completion},
                    {Module51ActionKind::start_transfer},
                    {Module51ActionKind::wait_without_timeout, 0, 0, false, 1}};
    return plan;
}

Module53Plan plan_module53_command(const Module53Input& input) {
    return plan_module51_command(input);
}

Module54Plan plan_module54_command(const Module54Input& input) {
    return plan_module51_command(input);
}

Module55Plan plan_module55_command(const Module55Input& input) {
    auto plan = plan_module51_command(input);
    if (input.command == 0x0e || input.command == 0x0f) {
        for (auto& action : plan.actions) {
            if (action.kind == Module51ActionKind::bounded_wait) {
                action.stops_when_service_status_is_one = true;
            }
        }
    }
    return plan;
}

Module56Plan plan_module56_command(const Module56Input& input) {
    return plan_module51_command(input);
}

Module57Plan plan_module57_command(const Module57Input& input) {
    return plan_module51_command(input);
}

Module58Plan plan_module58_command(const Module58Input& input) {
    return plan_module51_command(input);
}

} // namespace echo::transport
