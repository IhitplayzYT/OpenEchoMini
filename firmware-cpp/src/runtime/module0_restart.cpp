#include "echo/runtime/module0_restart.hpp"

namespace echo::runtime {
namespace {

constexpr Module0RestartPlan kRestartPlan{
    .register_writes = {{
        {0xe000ed24U, 0x00000000U},
        {0xe000e010U, 0x00000000U},
        {0xe000e180U, 0xffffffffU},
        {0xe000e184U, 0xffffffffU},
        {0xe000e188U, 0xffffffffU},
        {0xe000e18cU, 0xffffffffU},
        {0xe000e190U, 0xffffffffU},
        {0xe000e194U, 0xffffffffU},
        {0xe000e198U, 0xffffffffU},
        {0xe000e19cU, 0xffffffffU},
        {0xe000ed08U, 0x03050000U},
        {0xe000ed0cU, 0x05fa0002U},
    }},
    .zero_fill_entry = 0x03060bbcU,
    .zero_fill_address = 0x03004a20U,
    .zero_fill_length = 0x00019cb4U,
    .vector_table_address = 0x03050000U,
};

} // namespace

const Module0RestartPlan& module0_restart_plan() noexcept {
    return kRestartPlan;
}

} // namespace echo::runtime
