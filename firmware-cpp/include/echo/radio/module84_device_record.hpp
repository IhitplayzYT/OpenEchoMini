#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::radio {

inline constexpr std::size_t kModule84DeviceSlots = 6;
inline constexpr std::size_t kModule84DeviceMetadataBytes = 6;
inline constexpr std::size_t kModule84DeviceNameBytes = 0x20;
inline constexpr std::size_t kModule84DeviceRecordBytes = 0x26;

struct Module84DeviceListRecord {
    std::array<std::uint8_t, kModule84DeviceMetadataBytes> opaque_metadata{};
    std::array<std::uint8_t, kModule84DeviceNameBytes> utf8_name{};
};

// Parses one of the six 0x26-byte records reset by FUN_0307b5c6 and read by
// FUN_0307b842. Metadata stays opaque; the name starts at record offset +6.
[[nodiscard]] std::optional<Module84DeviceListRecord>
decode_module84_device_list_record(
    std::span<const std::uint8_t> bytes) noexcept;

// Uses the existing bounded UTF-8 subset decoder on the name field through
// its first NUL, or all 32 bytes when no NUL is present.
[[nodiscard]] bool decode_module84_device_list_name(
    const Module84DeviceListRecord& record,
    std::vector<char16_t>& output);

} // namespace echo::radio
