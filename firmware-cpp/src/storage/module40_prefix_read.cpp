#include "echo/storage/module40_prefix_read.hpp"

namespace echo::storage {

std::optional<Module40PrefixReadPlan> plan_module40_prefix_read(
    const Module40RegionMode mode, const std::uint32_t storage_sector_base,
    const std::uint32_t record_index, const std::uint32_t group_record_offset,
    const std::uint32_t record_block_id,
    const std::uint32_t prefix_byte_offset) noexcept {
    std::uint32_t record_region_sector = 0;
    std::uint32_t prefix_region_sector = 0;
    switch (mode) {
    case Module40RegionMode::first:
        record_region_sector = 0x40000;
        break;
    case Module40RegionMode::second:
        record_region_sector = 0x44000;
        prefix_region_sector = 0x20000;
        break;
    default:
        return std::nullopt;
    }

    const auto header_sector = storage_sector_base + record_region_sector;
    const auto header_index = record_index + group_record_offset;
    const auto prefix_sector = storage_sector_base + prefix_region_sector;
    return Module40PrefixReadPlan{
        header_sector * 0x200u + header_index * 0x10u,
        0x10,
        prefix_sector * 0x200u + record_block_id * 0x800u + prefix_byte_offset,
        8,
    };
}

} // namespace echo::storage
