#pragma once

#include <cstdint>
#include <vector>

namespace echo::storage {

enum class Module8TransferOperationKind : std::uint8_t {
    overlay_entry,
    device_key_0x30000,
};

struct Module8TransferOperation {
    Module8TransferOperationKind kind{};
    // The firmware passes this 32-bit sector/index value without naming its
    // direction. Arithmetic follows the recovered 32-bit register behavior.
    std::uint32_t sector_index{};
    std::uint8_t sector_count{16};
};

struct Module8TransferPlan {
    static constexpr std::uint32_t sector_bytes = 0x200U;
    static constexpr std::uint32_t window_bytes = 0x2000U;
    static constexpr std::uint8_t window_sectors = 16;

    std::vector<Module8TransferOperation> operations;
};

// Reproduces the operation order in module 8's 16-sector transfer helper.
// `overlay_entry` is kept opaque because the overlay entry at 0x0307c0e8 is
// not decoded well enough to assign it a read/write direction. This planner
// does no I/O and does not model the separate file-open path.
[[nodiscard]] Module8TransferPlan plan_module8_block_transfer(
    std::uint32_t first_index, std::uint32_t second_index,
    std::uint32_t extent_bytes);

} // namespace echo::storage
