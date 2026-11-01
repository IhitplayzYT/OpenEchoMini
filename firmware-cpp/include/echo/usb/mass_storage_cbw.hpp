#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::usb {

enum class HostDataDirection : std::uint8_t {
    device_to_host,
    host_to_device,
};

struct MassStorageCommandBlockWrapper {
    std::uint32_t tag{};
    std::uint32_t transfer_length{};
    std::uint8_t flags{};
    std::uint8_t logical_unit{};
    std::uint8_t command_length{};
    std::array<std::uint8_t, 16> command{};

    [[nodiscard]] bool has_data_stage() const noexcept { return transfer_length != 0; }
    [[nodiscard]] HostDataDirection direction() const noexcept {
        return (flags & 0x80U) != 0 ? HostDataDirection::device_to_host
                                    : HostDataDirection::host_to_device;
    }
};

enum class MassStorageBlockOperation : std::uint8_t { read10, write10 };

enum class MassStorageCommandKind : std::uint8_t {
    test_unit_ready,
    request_sense,
    read_format_capacities,
    inquiry,
    mode_select6,
    mode_sense6,
    start_stop_unit,
    prevent_allow_medium_removal,
    read_capacity10,
    read10,
    write10,
    mode_select10,
    mode_sense10,
    vendor_specific,
    unknown,
};

struct MassStorageBlockCommand {
    MassStorageBlockOperation operation{};
    std::uint32_t logical_block_address{};
    std::uint16_t block_count{};
    std::uint32_t transfer_bytes{};
};

// Parses the 31-byte BOT command block layout used by module 12. Signature,
// command-length and packet bounds are checked; reserved-bit acceptance is
// left to the caller.
[[nodiscard]] std::optional<MassStorageCommandBlockWrapper> parse_mass_storage_cbw(
    std::span<const std::uint8_t> bytes) noexcept;

// Decodes the READ(10)/WRITE(10) LBA and block count used by module 12.
[[nodiscard]] std::optional<MassStorageBlockCommand> parse_mass_storage_block_command(
    std::span<const std::uint8_t> cdb) noexcept;

// Names the command opcodes with explicit branches in module 12.
[[nodiscard]] MassStorageCommandKind classify_mass_storage_opcode(
    std::uint8_t opcode) noexcept;

} // namespace echo::usb
