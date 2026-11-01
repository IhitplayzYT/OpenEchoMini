#include "echo/text/firmware_codepage_tables.hpp"

namespace echo::text {
namespace {

constexpr std::size_t kTableStride = 0x20000;
constexpr std::size_t kTableCount = 14;

} // namespace

std::optional<std::uint16_t> lookup_firmware_codepage(
    const std::span<const std::uint8_t> region,
    const FirmwareCodepage codepage,
    const std::uint16_t input_unit) noexcept {
    const auto table_index = static_cast<std::size_t>(codepage);
    if (table_index >= kTableCount) return std::nullopt;

    const auto slot_offset = table_index * kTableStride +
                             static_cast<std::size_t>(input_unit) * 2;
    if (slot_offset > region.size() || region.size() - slot_offset < 2) {
        return std::nullopt;
    }

    const auto value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(region[slot_offset]) << 8U) |
        static_cast<std::uint16_t>(region[slot_offset + 1]));
    if (input_unit != 0 && value == 0) return std::nullopt;
    return value;
}

} // namespace echo::text
