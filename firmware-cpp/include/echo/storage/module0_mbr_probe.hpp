#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule0MbrBytes = 512U;
inline constexpr std::size_t kModule0MbrPartitionTableOffset = 0x1beU;

struct Module0MbrProbeResult {
    bool accepted_by_firmware_check{};
    std::uint32_t first_partition_start_lba{};
};

// Mirrors FUN_030583be's signature and four-entry consistency check, then
// returns the first entry's start LBA as FUN_030584e8 reads it.
[[nodiscard]] Module0MbrProbeResult inspect_module0_mbr_first_partition(
    std::span<const std::uint8_t, kModule0MbrBytes> sector) noexcept;

} // namespace echo::storage
