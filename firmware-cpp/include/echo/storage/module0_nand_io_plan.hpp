#pragma once

#include <array>
#include <cstdint>

namespace echo::storage {

enum class Module0NandIoKind : std::uint8_t { read, write };
enum class Module0NandIoStatus : std::uint8_t {
    ready,
    invalid_selector,
    write_gate_rejected,
    out_of_bounds,
};

struct Module0NandIoPlan {
    Module0NandIoStatus status{Module0NandIoStatus::invalid_selector};
    std::uint64_t partition_sector{};
    std::uint64_t backend_sector{};
    std::uint32_t sector_count{};
    bool backend_nonzero_status_becomes_error{};
};

// Models the observed key-3 NAND callback's partition adjustment, 0x2000
// sector prefix and range gate. It performs no storage I/O.
[[nodiscard]] inline Module0NandIoPlan plan_module0_nand_io(
    Module0NandIoKind kind, std::uint8_t selector, std::uint32_t sector,
    std::uint32_t sector_count, const std::array<std::uint32_t, 3>& offsets,
    std::uint32_t usable_sectors, std::uint32_t first_partition_sector,
    bool nand_signature_present) noexcept {
    if (selector > 3) return {};

    std::uint64_t adjusted = sector;
    for (std::uint8_t i = 0; i < selector; ++i) adjusted += offsets[i];

    if (kind == Module0NandIoKind::write && !nand_signature_present
        && adjusted < first_partition_sector) {
        return {Module0NandIoStatus::write_gate_rejected, adjusted, 0,
                sector_count, true};
    }

    if (adjusted + sector_count > usable_sectors) {
        return {Module0NandIoStatus::out_of_bounds, adjusted, 0,
                sector_count, true};
    }

    return {Module0NandIoStatus::ready, adjusted,
            adjusted + 0x2000, sector_count, true};
}

} // namespace echo::storage
