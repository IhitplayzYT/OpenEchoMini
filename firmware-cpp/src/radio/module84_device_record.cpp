#include "echo/radio/module84_device_record.hpp"

#include "echo/display/utf8_bmp.hpp"

#include <algorithm>
#include <iterator>

namespace echo::radio {

std::optional<Module84DeviceListRecord> decode_module84_device_list_record(
    const std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < kModule84DeviceRecordBytes) return std::nullopt;

    Module84DeviceListRecord record;
    std::copy_n(bytes.begin(), kModule84DeviceMetadataBytes,
                record.opaque_metadata.begin());
    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(kModule84DeviceMetadataBytes),
                kModule84DeviceNameBytes, record.utf8_name.begin());
    return record;
}

bool decode_module84_device_list_name(
    const Module84DeviceListRecord& record,
    std::vector<char16_t>& output) {
    const auto terminator =
        std::find(record.utf8_name.begin(), record.utf8_name.end(), 0U);
    const auto length = static_cast<std::size_t>(
        std::distance(record.utf8_name.begin(), terminator));
    return echo::display::decode_utf8_bmp(
        std::span<const std::uint8_t>(record.utf8_name.data(), length), output);
}

} // namespace echo::radio
