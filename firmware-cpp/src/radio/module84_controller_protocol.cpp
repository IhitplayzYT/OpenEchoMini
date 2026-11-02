#include "echo/radio/module84_controller_protocol.hpp"

namespace echo::radio {

std::optional<std::uint8_t> parse_module84_cv1_reply(
    const std::span<const std::uint8_t> reply) noexcept {
    if (reply.size() < 4U || reply[0] != 'C' || reply[1] != 'V'
        || reply[2] != '1') {
        return std::nullopt;
    }
    return reply[3] == '5' ? std::uint8_t{5} : std::uint8_t{0};
}

} // namespace echo::radio
