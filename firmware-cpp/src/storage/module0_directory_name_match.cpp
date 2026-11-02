#include "echo/storage/module0_directory_name_match.hpp"

namespace echo::storage {

namespace {

[[nodiscard]] std::uint16_t read_u16_le(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint16_t>(bytes[0])
         | static_cast<std::uint16_t>(
               static_cast<std::uint16_t>(bytes[1]) << 8U);
}

} // namespace

Module0DirectoryNameMatch match_module0_directory_name_record(
    const std::span<const std::uint8_t, kModule0DirectoryNameRecordBytes> entry,
    const std::span<const std::uint8_t, kModule0DirectoryShortNameBytes> requested_name,
    const std::uint8_t backend_selector) noexcept {
    if (entry[0] == 0U) {
        return Module0DirectoryNameMatch::end_of_directory;
    }

    if (backend_selector == 3U) {
        if (entry[0] != 0xc1U) {
            return Module0DirectoryNameMatch::ignored_entry;
        }
        for (std::size_t i = 0; i < requested_name.size(); ++i) {
            if (read_u16_le(entry.data() + 2U + i * 2U)
                != requested_name[i]) {
                return Module0DirectoryNameMatch::mismatch;
            }
        }
        return Module0DirectoryNameMatch::match;
    }

    if ((entry[0x0bU] & 0x08U) != 0U) {
        return Module0DirectoryNameMatch::ignored_entry;
    }
    for (std::size_t i = 0; i < requested_name.size(); ++i) {
        if (entry[i] != requested_name[i]) {
            return Module0DirectoryNameMatch::mismatch;
        }
    }
    return Module0DirectoryNameMatch::match;
}

} // namespace echo::storage
