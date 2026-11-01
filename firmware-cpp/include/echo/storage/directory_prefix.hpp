#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace echo::storage {

struct DirectoryPrefixRecord {
    std::array<std::uint16_t, 4> prefix{};
    std::uint16_t name_record_index{};

    // Decodes the name key from the observed 14-byte in-memory list node.
    // The leading 32-bit link pointer is intentionally not exposed.
    [[nodiscard]] static DirectoryPrefixRecord decode_runtime_node(
        std::span<const std::uint8_t> bytes);

    // Decodes the four UTF-16 units read from the block-backed prefix region.
    [[nodiscard]] static DirectoryPrefixRecord decode_prefix_bytes(
        std::span<const std::uint8_t> bytes,
        std::uint16_t name_record_index = 0);
};

} // namespace echo::storage
