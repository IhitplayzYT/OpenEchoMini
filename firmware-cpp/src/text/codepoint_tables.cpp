#include "echo/text/codepoint_tables.hpp"

namespace echo::text {

std::optional<std::uint16_t> read_codepoint_table_slot(
    std::span<const std::uint8_t> table,
    std::size_t slot) noexcept {
    if (slot > (table.size() / 2) || table.size() / 2 - slot < 1) {
        return std::nullopt;
    }
    const auto offset = slot * 2;
    const auto codepoint = static_cast<std::uint16_t>(table[offset]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(table[offset + 1]) << 8U);
    if (codepoint == 0) return std::nullopt;
    return codepoint;
}

std::optional<std::size_t> find_codepoint_table_slot(
    std::span<const std::uint8_t> table,
    std::uint16_t codepoint) noexcept {
    if (codepoint == 0) return std::nullopt;
    const auto slots = table.size() / 2;
    for (std::size_t slot = 0; slot < slots; ++slot) {
        const auto value = read_codepoint_table_slot(table, slot);
        if (value && *value == codepoint) return slot;
    }
    return std::nullopt;
}

std::optional<std::uint16_t> lookup_high_byte_codepoint(
    std::span<const std::uint8_t> table,
    std::uint8_t value) noexcept {
    if (value < 0x80) return std::nullopt;
    return read_codepoint_table_slot(table, static_cast<std::size_t>(value - 0x80));
}

std::optional<std::uint16_t> lookup_double_byte_codepoint(
    std::span<const std::uint8_t> table,
    DoubleByteTableLayout layout,
    std::uint8_t lead,
    std::uint8_t trail) noexcept {
    std::size_t lead_base = 0;
    std::size_t trail_offset = 0;
    bool valid_trail = false;

    switch (layout) {
    case DoubleByteTableLayout::cp932:
        if (!((lead >= 0x81 && lead <= 0x9f) ||
              (lead >= 0xe0 && lead <= 0xfc))) {
            return std::nullopt;
        }
        lead_base = 0x81;
        if (trail >= 0x40 && trail <= 0x7e) {
            trail_offset = static_cast<std::size_t>(trail - 0x40);
            valid_trail = true;
        } else if (trail >= 0x80 && trail <= 0xfc) {
            trail_offset = static_cast<std::size_t>(trail - 0x40);
            valid_trail = true;
        }
        break;
    case DoubleByteTableLayout::big5:
        if (lead < 0xa1 || lead > 0xf9) return std::nullopt;
        lead_base = 0xa1;
        if ((trail >= 0x40 && trail <= 0x7e) ||
            (trail >= 0xa1 && trail <= 0xfe)) {
            trail_offset = static_cast<std::size_t>(trail - 0x40);
            valid_trail = true;
        }
        break;
    case DoubleByteTableLayout::cp949:
        if (lead < 0x81 || lead > 0xfe) return std::nullopt;
        lead_base = 0x81;
        if (trail >= 0x41 && trail <= 0x5a) {
            trail_offset = static_cast<std::size_t>(trail - 0x41);
            valid_trail = true;
        } else if (trail >= 0x61 && trail <= 0x7a) {
            trail_offset = static_cast<std::size_t>(trail - 0x41);
            valid_trail = true;
        } else if (trail >= 0x81 && trail <= 0xfe) {
            trail_offset = static_cast<std::size_t>(trail - 0x41);
            valid_trail = true;
        }
        break;
    case DoubleByteTableLayout::cp949_hanja:
        if (lead < 0xca || lead > 0xfd || trail < 0xa1 || trail > 0xfe) {
            return std::nullopt;
        }
        lead_base = 0xca;
        trail_offset = static_cast<std::size_t>(trail - 0xa1);
        valid_trail = true;
        break;
    }

    if (!valid_trail || lead < lead_base) return std::nullopt;
    const auto slot = (static_cast<std::size_t>(lead - lead_base) * 256U) +
        trail_offset;
    return read_codepoint_table_slot(table, slot);
}

} // namespace echo::text
