#include "echo/storage/module0_queued_wide_name.hpp"

#include <cstddef>

namespace echo::storage {

std::optional<Module0QueuedWideName> decode_module0_queued_wide_name(
    const std::span<const std::uint8_t> marker_record,
    const std::span<const std::uint8_t> continuation_records) noexcept {
    if (marker_record.size() < kModule0DirectoryRecordBytes
        || marker_record[0] != 0xc0U) {
        return std::nullopt;
    }

    const auto unit_count = static_cast<std::size_t>(marker_record[3]);
    if (unit_count + 1U > kModule0QueuedNameOutputBytes / sizeof(char16_t)) {
        return std::nullopt;
    }

    const auto record_count =
        (unit_count + kModule0QueuedNameUnitsPerRecord - 1U)
        / kModule0QueuedNameUnitsPerRecord;
    const auto required_bytes = record_count * kModule0DirectoryRecordBytes;
    if (continuation_records.size() < required_bytes) return std::nullopt;

    Module0QueuedWideName name;
    name.length = static_cast<std::uint8_t>(unit_count);
    for (std::size_t i = 0; i < unit_count; ++i) {
        const auto record_offset =
            (i / kModule0QueuedNameUnitsPerRecord) * kModule0DirectoryRecordBytes;
        const auto unit_offset =
            record_offset + 2U + (i % kModule0QueuedNameUnitsPerRecord) * 2U;
        const auto value = static_cast<std::uint16_t>(
            continuation_records[unit_offset]
            | (static_cast<std::uint16_t>(continuation_records[unit_offset + 1U])
               << 8U));
        name.code_units[i] = static_cast<char16_t>(value);
    }
    name.code_units[unit_count] = u'\0';
    return name;
}

} // namespace echo::storage
