#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

enum class Module0BootSectorKind : std::uint8_t {
    unrecognized,
    exfat_prefix,
    fat_bpb_candidate,
};

inline constexpr std::size_t kModule0BootSectorBytes = 512U;

// Reproduces FUN_030584ae's exact signature and BPB-candidate checks. The
// exFAT label means the firmware's 11-byte prefix matched, not full validation.
[[nodiscard]] Module0BootSectorKind classify_module0_boot_sector(
    std::span<const std::uint8_t, kModule0BootSectorBytes> sector) noexcept;

struct Module0ExfatGeometry {
    std::uint32_t volume_length_sectors{};
    std::uint32_t cluster_heap_sector_offset{};
    std::uint32_t cluster_heap_end_sector_offset{};
    std::uint32_t data_cluster_count{};
    std::uint32_t root_directory_cluster{};
    std::uint32_t bytes_per_sector{};
    std::uint32_t sectors_per_cluster{};
    std::uint8_t percent_in_use{};
};

// Parses the fields FUN_030584e8 consumes from the exFAT BPB and applies its
// observed revision, FAT-count, cluster-limit, and volume-length checks.
[[nodiscard]] std::optional<Module0ExfatGeometry>
parse_module0_exfat_geometry(
    std::span<const std::uint8_t, kModule0BootSectorBytes> sector) noexcept;

struct Module0ExfatFatEntryLocation {
    std::uint32_t sector_lba{};
    std::uint16_t byte_offset{};
};

// Models FUN_03058b1c's selector-3 (exFAT) 4-byte FAT-entry address formula.
[[nodiscard]] std::optional<Module0ExfatFatEntryLocation>
locate_module0_exfat_fat_entry(std::uint32_t cluster_index,
                               std::uint32_t fat_start_lba,
                               std::uint8_t sector_shift,
                               std::uint16_t bytes_per_sector) noexcept;

inline constexpr std::uint32_t kModule0FatChainUpperSentinel = 0x0ffffff7U;

// True only for values FUN_03058d7e allows to continue as a cluster index.
[[nodiscard]] bool module0_fat_chain_step_is_in_range(
    std::uint32_t next_cluster) noexcept;

// The numeric values match FUN_0305826e's stored selector. Their FAT12/16/32
// labels are inferred from its BPB fields and standard cluster-count cutoffs.
enum class Module0FatVolumeKind : std::uint8_t {
    fat12 = 0,
    fat16 = 1,
    fat32 = 2,
};

inline constexpr std::uint32_t kModule0Fat12ClusterLimit = 0x0ff5U;
inline constexpr std::uint32_t kModule0Fat32ClusterStart = 0x0fff5U;

// Models the volume-type decision after the firmware derives a cluster count.
// Geometry calculation remains separate.
[[nodiscard]] Module0FatVolumeKind classify_module0_fat_volume(
    std::uint32_t data_cluster_count) noexcept;

} // namespace echo::storage
