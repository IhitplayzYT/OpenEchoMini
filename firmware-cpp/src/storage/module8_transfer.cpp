#include "echo/storage/module8_transfer.hpp"

#include <cstddef>
#include <cstdint>

namespace echo::storage {
namespace {

constexpr std::uint32_t add_sector_index(const std::uint32_t base,
                                         const std::uint64_t delta) noexcept {
    return base + static_cast<std::uint32_t>(delta);
}

void append(Module8TransferPlan& plan,
            const Module8TransferOperationKind kind,
            const std::uint32_t sector_index) {
    plan.operations.push_back(
        {kind, sector_index, Module8TransferPlan::window_sectors});
}

} // namespace

Module8TransferPlan plan_module8_block_transfer(
    const std::uint32_t first_index, const std::uint32_t second_index,
    const std::uint32_t extent_bytes) {
    Module8TransferPlan plan{};
    append(plan, Module8TransferOperationKind::overlay_entry, first_index);

    std::uint64_t loop_count = 0;
    if (extent_bytes > Module8TransferPlan::window_bytes) {
        const auto loop_limit = static_cast<std::uint64_t>(
            extent_bytes - Module8TransferPlan::window_bytes);
        loop_count = (loop_limit + Module8TransferPlan::window_bytes - 1U) /
                     Module8TransferPlan::window_bytes;
    }
    for (std::uint64_t i = 0; i < loop_count; ++i) {
        const auto sector_delta = Module8TransferPlan::window_sectors * (i + 1U);
        append(plan, Module8TransferOperationKind::device_key_0x30000,
               add_sector_index(second_index, sector_delta));
        append(plan, Module8TransferOperationKind::overlay_entry,
               add_sector_index(first_index, sector_delta));
    }

    const auto trailing_sectors = extent_bytes / Module8TransferPlan::sector_bytes;
    const auto final_delta = static_cast<std::uint32_t>(
        Module8TransferPlan::window_sectors - trailing_sectors);
    append(plan, Module8TransferOperationKind::device_key_0x30000,
           add_sector_index(second_index, final_delta));
    append(plan, Module8TransferOperationKind::overlay_entry,
           add_sector_index(first_index, final_delta));
    return plan;
}

} // namespace echo::storage
