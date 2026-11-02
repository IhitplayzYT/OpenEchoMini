#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::radio {

inline constexpr std::uint8_t kModule84ScanRequestByte = 0x53;
inline constexpr std::uint16_t kModule84Cl06EventCode = 10;

struct Module84Cl06Command {
    std::array<std::uint8_t, 10> bytes{};
};

// FUN_0307b5c6 writes the four-byte "CL06" prefix followed by six opaque
// bytes from the selected-device state record.
[[nodiscard]] constexpr Module84Cl06Command make_module84_cl06_command(
    const std::array<std::uint8_t, 6>& selected_device_token) noexcept {
    Module84Cl06Command command{{'C', 'L', '0', '6'}};
    for (std::size_t i = 0; i < selected_device_token.size(); ++i) {
        command.bytes[i + 4U] = selected_device_token[i];
    }
    return command;
}

// A matched "CV1" reply maps suffix '5' to result 5 and every other suffix to
// result 0. A null result means the bounded reply did not match the prefix.
[[nodiscard]] std::optional<std::uint8_t> parse_module84_cv1_reply(
    std::span<const std::uint8_t> reply) noexcept;

} // namespace echo::radio
