#pragma once

#include <cstdint>
#include <span>

namespace echo::storage {

// Applies the record-type-specific XOR transform observed in FUN_0308ce58.
// The source firmware's 512-byte lookup region is embedded in the
// reconstruction. Invalid records or undersized host spans return zero.
[[nodiscard]] std::int32_t transform_module59_record(
    std::span<std::uint8_t> bytes,
    std::int32_t length,
    std::span<const std::uint8_t> record,
    std::uint32_t stream_offset,
    std::uint32_t key_offset);

} // namespace echo::storage
