#include "echo/storage/module40_listed_prefix_read.hpp"

namespace echo::storage {

Module40ListedPrefixReadPlan plan_module40_listed_prefix_read(
    const std::uint32_t storage_sector_base, const std::uint32_t item_or_index,
    const std::uint32_t prefix_byte_offset,
    const bool use_index_list) noexcept {
    const auto name_index = static_cast<std::uint16_t>(item_or_index);
    const auto record_index = use_index_list ? name_index :
                              static_cast<std::uint16_t>(item_or_index);
    const auto byte_offset = storage_sector_base * 0x200u +
                             static_cast<std::uint32_t>(record_index) * 0x800u +
                             prefix_byte_offset;
    return {byte_offset, 8, record_index};
}

} // namespace echo::storage
