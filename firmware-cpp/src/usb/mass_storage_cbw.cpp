#include "echo/usb/mass_storage_cbw.hpp"

namespace echo::usb {
namespace {

std::uint32_t read_le32(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::uint32_t read_be32(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(bytes[offset]) << 24U)
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8U)
        | static_cast<std::uint32_t>(bytes[offset + 3]);
}

std::uint16_t read_be16(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[offset]) << 8U)
        | static_cast<std::uint16_t>(bytes[offset + 1]));
}

} // namespace

std::optional<MassStorageCommandBlockWrapper> parse_mass_storage_cbw(
    const std::span<const std::uint8_t> bytes) noexcept {
    constexpr std::size_t cbw_size = 31;
    constexpr std::uint32_t cbw_signature = 0x43425355U;
    constexpr std::size_t cdb_offset = 15;
    if (bytes.size() < cbw_size || read_le32(bytes, 0) != cbw_signature
        || bytes[14] == 0 || bytes[14] > 16) {
        return std::nullopt;
    }

    MassStorageCommandBlockWrapper result{};
    result.tag = read_le32(bytes, 4);
    result.transfer_length = read_le32(bytes, 8);
    result.flags = bytes[12];
    result.logical_unit = bytes[13];
    result.command_length = bytes[14];
    for (std::size_t i = 0; i < result.command.size(); ++i) {
        result.command[i] = bytes[cdb_offset + i];
    }
    return result;
}

std::optional<MassStorageBlockCommand> parse_mass_storage_block_command(
    const std::span<const std::uint8_t> cdb) noexcept {
    constexpr std::size_t cdb10_size = 10;
    constexpr std::uint32_t bytes_per_block = 512;
    if (cdb.size() < cdb10_size) {
        return std::nullopt;
    }

    MassStorageBlockOperation operation{};
    if (cdb[0] == 0x28) {
        operation = MassStorageBlockOperation::read10;
    } else if (cdb[0] == 0x2a) {
        operation = MassStorageBlockOperation::write10;
    } else {
        return std::nullopt;
    }

    const auto block_count = read_be16(cdb, 7);
    return MassStorageBlockCommand{
        operation,
        read_be32(cdb, 2),
        block_count,
        static_cast<std::uint32_t>(block_count) * bytes_per_block,
    };
}

MassStorageCommandKind classify_mass_storage_opcode(const std::uint8_t opcode) noexcept {
    switch (opcode) {
    case 0x00: return MassStorageCommandKind::test_unit_ready;
    case 0x03: return MassStorageCommandKind::request_sense;
    case 0x12: return MassStorageCommandKind::inquiry;
    case 0x15: return MassStorageCommandKind::mode_select6;
    case 0x1a: return MassStorageCommandKind::mode_sense6;
    case 0x1b: return MassStorageCommandKind::start_stop_unit;
    case 0x1e: return MassStorageCommandKind::prevent_allow_medium_removal;
    case 0x23: return MassStorageCommandKind::read_format_capacities;
    case 0x25: return MassStorageCommandKind::read_capacity10;
    case 0x28: return MassStorageCommandKind::read10;
    case 0x2a: return MassStorageCommandKind::write10;
    case 0x55: return MassStorageCommandKind::mode_select10;
    case 0x5a: return MassStorageCommandKind::mode_sense10;
    case 0xff: return MassStorageCommandKind::vendor_specific;
    default: return MassStorageCommandKind::unknown;
    }
}

} // namespace echo::usb
