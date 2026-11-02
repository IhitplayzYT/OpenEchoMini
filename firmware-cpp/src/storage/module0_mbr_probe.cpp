#include "echo/storage/module0_mbr_probe.hpp"

namespace echo::storage {

Module0MbrProbeResult inspect_module0_mbr_first_partition(
    const std::span<const std::uint8_t, kModule0MbrBytes> sector) noexcept {
    if (sector[0x1feU] != 0x55U || sector[0x1ffU] != 0xaaU) return {};

    for (std::size_t i = 0; i < 4U; ++i) {
        const auto entry = kModule0MbrPartitionTableOffset + i * 0x10U;
        const bool status_or_first_chs_nonzero =
            sector[entry] != 0U || sector[entry + 1U] != 0U;
        if (status_or_first_chs_nonzero && sector[entry + 4U] == 0U) return {};
    }

    constexpr std::size_t first_start_lba_offset =
        kModule0MbrPartitionTableOffset + 8U;
    const auto first_lba = static_cast<std::uint32_t>(sector[first_start_lba_offset]) |
        (static_cast<std::uint32_t>(sector[first_start_lba_offset + 1U]) << 8U) |
        (static_cast<std::uint32_t>(sector[first_start_lba_offset + 2U]) << 16U) |
        (static_cast<std::uint32_t>(sector[first_start_lba_offset + 3U]) << 24U);
    return {true, first_lba};
}

} // namespace echo::storage
