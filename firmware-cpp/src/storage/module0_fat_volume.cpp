#include "echo/storage/module0_fat_volume.hpp"

namespace echo::storage {

namespace {

std::uint32_t read_le32(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
        (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
        (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

std::uint64_t read_le64(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept {
    std::uint64_t value = 0U;
    for (std::size_t i = 0; i < 8U; ++i) {
        value |= static_cast<std::uint64_t>(bytes[offset + i]) << (8U * i);
    }
    return value;
}

} // namespace

Module0BootSectorKind classify_module0_boot_sector(
    const std::span<const std::uint8_t, kModule0BootSectorBytes> sector) noexcept {
    if (sector[0x1feU] != 0x55U || sector[0x1ffU] != 0xaaU) {
        return Module0BootSectorKind::unrecognized;
    }

    constexpr std::uint8_t exfat_prefix[11] = {
        0xebU, 0x76U, 0x90U, 'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '};
    bool matches_exfat_prefix = true;
    for (std::size_t i = 0; i < 11U; ++i) {
        if (sector[i] != exfat_prefix[i]) {
            matches_exfat_prefix = false;
            break;
        }
    }
    if (matches_exfat_prefix) return Module0BootSectorKind::exfat_prefix;

    const auto sectors_per_cluster = sector[0x0dU];
    if (sectors_per_cluster == 0U ||
        (sectors_per_cluster & static_cast<std::uint8_t>(sectors_per_cluster - 1U)) != 0U) {
        return Module0BootSectorKind::unrecognized;
    }

    const auto fat_count = sector[0x10U];
    if (fat_count != 1U && fat_count != 2U) return Module0BootSectorKind::unrecognized;

    const auto sectors_per_track = static_cast<std::uint16_t>(sector[0x18U]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(sector[0x19U]) << 8U);
    if (sectors_per_track == 0U || sectors_per_track > 0x3fU) {
        return Module0BootSectorKind::unrecognized;
    }

    const auto head_count = static_cast<std::uint16_t>(sector[0x1aU]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(sector[0x1bU]) << 8U);
    if (head_count == 0U || head_count > 0xffU) {
        return Module0BootSectorKind::unrecognized;
    }

    return Module0BootSectorKind::fat_bpb_candidate;
}

std::optional<Module0ExfatGeometry> parse_module0_exfat_geometry(
    const std::span<const std::uint8_t, kModule0BootSectorBytes> sector) noexcept {
    if (classify_module0_boot_sector(sector) != Module0BootSectorKind::exfat_prefix ||
        sector[0x68U] != 0x00U || sector[0x69U] != 0x01U ||
        sector[0x6eU] != 1U) {
        return std::nullopt;
    }

    const auto bytes_shift = sector[0x6cU];
    const auto cluster_shift = sector[0x6dU];
    if (bytes_shift >= 32U || cluster_shift >= 32U) return std::nullopt;

    const auto bytes_per_sector = std::uint32_t{1} << bytes_shift;
    const auto sectors_per_cluster = std::uint32_t{1} << cluster_shift;
    const auto volume_length = read_le64(sector, 0x48U);
    const auto cluster_heap = read_le32(sector, 0x58U);
    const auto cluster_count = read_le32(sector, 0x5cU);
    if (cluster_count > 0x7ffffffdU) return std::nullopt;

    // The firmware stores and compares the low 32-bit sector count here;
    // unsigned arithmetic retains the target's 32-bit wrap behavior.
    const auto volume_length_sectors = static_cast<std::uint32_t>(volume_length);
    const auto data_region_end = static_cast<std::uint32_t>(
        cluster_heap + cluster_count * sectors_per_cluster);
    if (data_region_end > volume_length_sectors) return std::nullopt;

    return Module0ExfatGeometry{
        volume_length_sectors,
        cluster_heap,
        data_region_end,
        cluster_count,
        read_le32(sector, 0x60U),
        bytes_per_sector,
        sectors_per_cluster,
        sector[0x70U],
    };
}

std::optional<Module0ExfatFatEntryLocation> locate_module0_exfat_fat_entry(
    const std::uint32_t cluster_index, const std::uint32_t fat_start_lba,
    const std::uint8_t sector_shift,
    const std::uint16_t bytes_per_sector) noexcept {
    if (sector_shift >= 32U) return std::nullopt;
    const auto byte_index = static_cast<std::uint32_t>(cluster_index << 2U);
    const auto sector_delta = byte_index >> sector_shift;
    const auto byte_offset = static_cast<std::uint16_t>(
        byte_index & static_cast<std::uint32_t>(bytes_per_sector - 1U));
    return Module0ExfatFatEntryLocation{
        static_cast<std::uint32_t>(fat_start_lba + sector_delta), byte_offset};
}

bool module0_fat_chain_step_is_in_range(const std::uint32_t next_cluster) noexcept {
    return next_cluster >= 2U && next_cluster < kModule0FatChainUpperSentinel;
}

Module0FatVolumeKind classify_module0_fat_volume(
    const std::uint32_t data_cluster_count) noexcept {
    if (data_cluster_count < kModule0Fat12ClusterLimit) {
        return Module0FatVolumeKind::fat12;
    }
    if (data_cluster_count >= kModule0Fat32ClusterStart) {
        return Module0FatVolumeKind::fat32;
    }
    return Module0FatVolumeKind::fat16;
}

} // namespace echo::storage
