#pragma once

#include <cstdint>

namespace echo::storage {

enum class Module0MediaTransferDirection : std::uint8_t { read, write };

struct Module0MediaTransferPlan {
    std::uint16_t command{};
    std::uint16_t stop_command{0x414c};
    std::uint32_t sector_argument{};
    std::uint32_t transfer_bytes{};
    std::uint16_t fixed_block_bytes{0x200};
    std::uint8_t stop_delay_ms{1};
    bool stop_after_transfer{};
};

[[nodiscard]] inline Module0MediaTransferPlan plan_module0_media_transfer(
    Module0MediaTransferDirection direction, std::uint32_t sector,
    std::uint32_t sector_count, std::int32_t callback_status) noexcept {
    const bool single = sector_count == 1;
    const auto command = direction == Module0MediaTransferDirection::read
        ? static_cast<std::uint16_t>(single ? 0x2351 : 0x2352)
        : static_cast<std::uint16_t>(single ? 0x2758 : 0x2759);
    const bool read_stop_status = callback_status == 0
        || callback_status == 0x100 || callback_status == 0x80
        || callback_status == 0x40;
    const bool write_stop_status = callback_status == 0
        || callback_status == 0x100 || callback_status == 0x40;
    const auto stop = direction == Module0MediaTransferDirection::read
        ? read_stop_status : write_stop_status;
    return {command, 0x414c, sector,
            single ? 0x200U : sector_count << 9U,
            0x200, 1, stop};
}

} // namespace echo::storage
