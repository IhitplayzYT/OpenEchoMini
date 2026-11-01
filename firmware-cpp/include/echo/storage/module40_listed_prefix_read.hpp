#pragma once

#include <cstdint>

namespace echo::storage {

struct Module40ListedPrefixReadPlan {
    std::uint32_t byte_offset{};
    std::uint32_t read_size{8};
    std::uint16_t name_record_index{};
};

// Models FUN_0307bac2. When use_index_list is true, item_or_index is read
// from the index list; otherwise it is the sequential item number.
[[nodiscard]] Module40ListedPrefixReadPlan plan_module40_listed_prefix_read(
    std::uint32_t storage_sector_base, std::uint32_t item_or_index,
    std::uint32_t prefix_byte_offset, bool use_index_list) noexcept;

} // namespace echo::storage
