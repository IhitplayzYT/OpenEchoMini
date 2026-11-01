#pragma once

#include <cstdint>
#include <optional>

namespace echo::storage {

enum class Module40RegionMode : std::uint8_t { first = 1, second = 2 };

struct Module40PrefixReadPlan {
    std::uint32_t record_header_offset{};
    std::uint32_t record_header_size{0x10};
    std::uint32_t prefix_offset{};
    std::uint32_t prefix_size{8};
};

// Computes the two block-byte offsets used by FUN_0307bb7c. Inputs include
// the storage base and the already-read record's block identifier.
[[nodiscard]] std::optional<Module40PrefixReadPlan> plan_module40_prefix_read(
    Module40RegionMode mode, std::uint32_t storage_sector_base,
    std::uint32_t record_index, std::uint32_t group_record_offset,
    std::uint32_t record_block_id, std::uint32_t prefix_byte_offset) noexcept;

} // namespace echo::storage
