#pragma once

#include "echo/storage/module0_directory_handle_close.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

inline constexpr std::uint32_t kModule0FileApiTableAddress = 0x03004578;
inline constexpr std::size_t kModule0FileApiCallbackCount = 8;

enum class Module0FileApiCallback : std::size_t {
    open = 0,
    read = 1,
    seek = 2,
    tell = 3,
    write = 4,
    length = 5,
    close = 6,
    eof = 7,
};

// Firmware virtual addresses, including the Thumb bit, written by
// FUN_03055558 into the callback table.
inline constexpr std::array<std::uint32_t, kModule0FileApiCallbackCount>
    kModule0FileApiCallbackAddresses{
        0x030597a7U,
        0x03059d8fU,
        0x0305a287U,
        0x03055547U,
        0x0305d871U,
        0x03055535U,
        0x03059c75U,
        0x0305a265U,
    };

inline constexpr std::size_t kModule0HandleCurrentPositionOffset = 0x20;

[[nodiscard]] inline std::uint32_t module0_file_api_read_u32_le(
    const std::span<const std::uint8_t, kModule0ResourceHandleSlotBytes> slot,
    const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(slot[offset])
        | (static_cast<std::uint32_t>(slot[offset + 1U]) << 8U)
        | (static_cast<std::uint32_t>(slot[offset + 2U]) << 16U)
        | (static_cast<std::uint32_t>(slot[offset + 3U]) << 24U);
}

[[nodiscard]] inline std::uint32_t module0_file_api_length(
    const std::span<const std::uint8_t, kModule0ResourceHandleSlotBytes> slot)
    noexcept {
    return module0_file_api_read_u32_le(slot, kModule0HandleLengthOffset);
}

[[nodiscard]] inline std::uint32_t module0_file_api_tell(
    const std::span<const std::uint8_t, kModule0ResourceHandleSlotBytes> slot)
    noexcept {
    return module0_file_api_read_u32_le(
        slot, kModule0HandleCurrentPositionOffset);
}

[[nodiscard]] inline bool module0_file_api_eof(
    const std::span<const std::uint8_t, kModule0ResourceHandleSlotBytes> slot)
    noexcept {
    return module0_file_api_tell(slot) >= module0_file_api_length(slot);
}

[[nodiscard]] constexpr std::uint32_t module0_file_api_callback_address(
    const Module0FileApiCallback callback) noexcept {
    return kModule0FileApiCallbackAddresses[
        static_cast<std::size_t>(callback)];
}

} // namespace echo::storage
