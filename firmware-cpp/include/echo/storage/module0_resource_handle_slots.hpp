#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

inline constexpr std::uint8_t kModule0ResourceHandleOpenState = 1;
inline constexpr std::uint8_t kModule0ResourceHandleSpecialModeState = 3;

inline constexpr std::size_t kModule0ResourceHandleSlotCount = 8;
inline constexpr std::size_t kModule0ResourceHandleSlotBytes = 0x27e;
inline constexpr std::size_t kModule0ResourceHandlePrefixBytes = 0x0c;
inline constexpr std::size_t kModule0ResourceHandleInputBytes = 11;

struct Module0ResourceHandleSlotPrefix {
    std::uint8_t state{};
    std::array<std::uint8_t, kModule0ResourceHandleInputBytes> input_bytes{};

    [[nodiscard]] constexpr bool is_free() const noexcept {
        return state == 0U;
    }
};

// Returns the byte offset of one of the eight 0x27e-byte slots. The firmware
// searches from slot zero and returns failure if all state bytes are nonzero.
[[nodiscard]] constexpr std::optional<std::size_t>
module0_resource_handle_slot_offset(const std::size_t index) noexcept {
    if (index >= kModule0ResourceHandleSlotCount) {
        return std::nullopt;
    }
    return index * kModule0ResourceHandleSlotBytes;
}

// FUN_03059aae switches a successful slot from state 1 to state 3 when either
// of the first two mode bytes is ASCII 'W'. Module 85 supplies the one-byte
// mode string "W" for its optional decoded-payload output path.
[[nodiscard]] inline constexpr bool module0_mode_requests_special_state(
    const std::span<const std::uint8_t> mode) noexcept {
    return !mode.empty()
        && (mode[0] == static_cast<std::uint8_t>('W')
            || (mode.size() > 1U
                && mode[1] == static_cast<std::uint8_t>('W')));
}

[[nodiscard]] inline constexpr std::uint8_t
module0_resource_handle_state_for_mode(
    const std::span<const std::uint8_t> mode) noexcept {
    return module0_mode_requests_special_state(mode)
        ? kModule0ResourceHandleSpecialModeState
        : kModule0ResourceHandleOpenState;
}

// Decodes only the observed prefix. The 11 bytes are copied from the open
// request in one branch; other slot modes can use the remainder differently.
[[nodiscard]] inline std::optional<Module0ResourceHandleSlotPrefix>
decode_module0_resource_handle_slot_prefix(
    const std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < kModule0ResourceHandlePrefixBytes) {
        return std::nullopt;
    }
    Module0ResourceHandleSlotPrefix result{};
    result.state = bytes[0];
    for (std::size_t i = 0; i < result.input_bytes.size(); ++i) {
        result.input_bytes[i] = bytes[i + 1U];
    }
    return result;
}

} // namespace echo::storage
