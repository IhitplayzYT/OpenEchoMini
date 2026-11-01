#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace echo::transport {

enum class Module51ActionKind : std::uint8_t {
    clear_completion,
    signal_event,
    submit_request,
    bounded_wait,
    start_transfer,
    wait_without_timeout,
};

struct Module51Action {
    Module51ActionKind kind{};
    std::uint32_t event_id{};
    std::uint32_t wait_limit{};
    bool uses_output_context{};
    std::uint8_t service_calls_per_pass{};
    bool stops_when_service_status_is_one{};
};

struct Module51Input {
    std::uint32_t command{};
    std::array<std::uint32_t, 12> state_words{};
    std::uint8_t buffer_toggle{};
    std::array<std::uint32_t, 8> descriptors{};
    std::uint32_t primary_buffer{};
    std::uint32_t secondary_buffer{};
    std::uint32_t command8_wait_limit{};
    std::uint32_t command14_wait_limit{};
    std::uint32_t command15_wait_limit{};
    bool command8_secondary_flag{};
};

struct Module51TransferPlan {
    std::uint32_t buffer_handle{};
    std::array<std::uint32_t, 4> descriptor{};
    std::uint32_t byte_count{};
    std::uint8_t toggle_after{};
};

struct Module51Plan {
    std::uint32_t result{};
    bool has_value_output{};
    std::uint32_t value_output{};
    bool has_count_output{};
    std::uint32_t count_output{};
    bool state9_written{};
    std::uint32_t state9_after{};
    bool state10_written{};
    std::uint32_t state10_after{};
    bool toggle_written{};
    std::uint8_t toggle_after{};
    bool transfer_wait_has_no_timeout{};
    std::vector<Module51Action> actions;
    Module51TransferPlan transfer{};
    bool has_transfer{};
};

// Plans the observed command behavior for module 51. Scheduler, event, and
// transfer operations remain adapter actions; command 10's wait is unbounded.
[[nodiscard]] Module51Plan plan_module51_command(const Module51Input& input);

// Module 53 has the same observed command mapping and toggle-to-buffer
// selection. Its initial toggle value is owned by its overlay.
using Module53Input = Module51Input;
using Module53Plan = Module51Plan;
[[nodiscard]] Module53Plan plan_module53_command(const Module53Input& input);
using Module54Input = Module51Input;
using Module54Plan = Module51Plan;
[[nodiscard]] Module54Plan plan_module54_command(const Module54Input& input);
using Module55Input = Module51Input;
using Module55Plan = Module51Plan;
[[nodiscard]] Module55Plan plan_module55_command(const Module55Input& input);
using Module56Input = Module51Input;
using Module56Plan = Module51Plan;
[[nodiscard]] Module56Plan plan_module56_command(const Module56Input& input);
using Module57Input = Module51Input;
using Module57Plan = Module51Plan;
[[nodiscard]] Module57Plan plan_module57_command(const Module57Input& input);
using Module58Input = Module51Input;
using Module58Plan = Module51Plan;
[[nodiscard]] Module58Plan plan_module58_command(const Module58Input& input);

} // namespace echo::transport
