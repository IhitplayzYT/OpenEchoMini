#include "echo/storage/module0_fat_directory_record.hpp"

namespace echo::storage {

bool normalize_module0_fat_directory_end_marker(
    const std::span<std::uint8_t, kModule0FatDirectoryRecordBytes> entry,
    const std::uint8_t backend_selector) noexcept {
    if (backend_selector == 3U || entry[kModule0FatDirectoryAttributesOffset] == 0x0fU) {
        return false;
    }
    for (std::size_t i = 0; i < 4U; ++i) {
        if (entry[i] != 0xffU) return false;
    }
    entry[0] = 0U;
    return true;
}

} // namespace echo::storage
