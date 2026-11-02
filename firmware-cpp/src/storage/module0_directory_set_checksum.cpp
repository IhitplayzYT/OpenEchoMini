#include "echo/storage/module0_directory_set_checksum.hpp"

namespace echo::storage {

bool write_module0_exfat_directory_set_checksum(
    const std::span<std::uint8_t> entry_set) noexcept {
    if (entry_set.size() < kModule0ExfatDirectoryEntryBytes
        || entry_set[0] != 0x85U) {
        return false;
    }

    const std::size_t entry_count = static_cast<std::size_t>(entry_set[1]) + 1U;
    if (entry_count > kModule0ExfatDirectoryMaximumEntryCount
        || entry_set.size() != entry_count * kModule0ExfatDirectoryEntryBytes) {
        return false;
    }

    std::uint16_t checksum = 0;
    for (std::size_t i = 0; i < entry_set.size(); ++i) {
        if (i == 2U || i == 3U) {
            continue;
        }
        const auto rotated = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(checksum >> 1U)
            | static_cast<std::uint16_t>(checksum << 15U));
        checksum = static_cast<std::uint16_t>(rotated + entry_set[i]);
    }

    entry_set[kModule0ExfatDirectoryChecksumOffset] =
        static_cast<std::uint8_t>(checksum & 0xffU);
    entry_set[kModule0ExfatDirectoryChecksumOffset + 1U] =
        static_cast<std::uint8_t>(checksum >> 8U);
    return true;
}

} // namespace echo::storage
