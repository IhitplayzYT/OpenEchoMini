#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

inline constexpr std::uint32_t kModule0SharedTimerCounterAddress = 0x0301c0b4U;

struct Module0BrowserNameKey {
    std::array<std::uint8_t, 11> bytes{};
    bool had_extension_separator{};
};

// Reconstructs FUN_0305d560's fixed 11-byte field from a UTF-16-like name.
// timer_tick_value supplies the shared value used for the three decimal
// digits. Module 0 stores a pointer to kModule0SharedTimerCounterAddress,
// then reduces that counter modulo 1000; its writer and cadence remain
// unresolved.
[[nodiscard]] std::optional<Module0BrowserNameKey>
format_module0_browser_name_key(
    std::span<const char16_t> name, std::uint32_t timer_tick_value) noexcept;

} // namespace echo::storage
