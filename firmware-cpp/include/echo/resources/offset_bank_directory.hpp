#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::resources {

struct OffsetBankDirectory {
    std::size_t count{};
    std::size_t header_bytes{};
};

struct ByteRange {
    std::size_t offset{};
    std::size_t length{};
};

// The region starts with a little-endian 16-bit count followed by that many
// little-endian 32-bit offsets. Offsets are relative to the region start.
[[nodiscard]] std::optional<OffsetBankDirectory> parse_offset_bank_directory(
    std::span<const std::uint8_t> region) noexcept;

[[nodiscard]] std::optional<ByteRange> offset_bank_range(
    std::span<const std::uint8_t> region,
    std::size_t bank_index) noexcept;

} // namespace echo::resources
