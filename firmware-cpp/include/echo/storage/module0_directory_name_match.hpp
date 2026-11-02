#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule0DirectoryNameRecordBytes = 0x20;
inline constexpr std::size_t kModule0DirectoryShortNameBytes = 11;

enum class Module0DirectoryNameMatch {
    end_of_directory,
    ignored_entry,
    mismatch,
    match,
};

// Models FUN_0305901e's bounded candidate comparison. Backend 3 examines
// 0xc1 filename-extension entries and widens each requested byte to compare
// against UTF-16 units beginning at entry offset +2. Other backends compare
// the 11-byte short-name field unless attribute bit 0x08 is set.
[[nodiscard]] Module0DirectoryNameMatch match_module0_directory_name_record(
    std::span<const std::uint8_t, kModule0DirectoryNameRecordBytes> entry,
    std::span<const std::uint8_t, kModule0DirectoryShortNameBytes> requested_name,
    std::uint8_t backend_selector) noexcept;

} // namespace echo::storage
