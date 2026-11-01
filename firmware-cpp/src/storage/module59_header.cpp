#include "echo/storage/module59_header.hpp"

namespace echo::storage {
namespace {

std::uint16_t read_u16(const std::span<const std::uint8_t> bytes, const std::size_t at) {
    return static_cast<std::uint16_t>(bytes[at]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[at + 1]) << 8);
}

std::uint32_t read_u32(const std::span<const std::uint8_t> bytes, const std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at]) |
           (static_cast<std::uint32_t>(bytes[at + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[at + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[at + 3]) << 24);
}

} // namespace

Module59HeaderParseResult parse_module59_header_prefix(
    const std::span<const std::uint8_t> bytes) {
    Module59HeaderParseResult result{};
    if (bytes.size() < 4) return result;

    auto& out = result.header;
    out.magic = read_u32(bytes, 0);
    if (out.magic != 0x20546156u && out.magic != 0x20544156u) {
        result.status = Module59HeaderStatus::invalid_magic;
        return result;
    }
    if (bytes.size() < 26) return result;

    out.type_primary = read_u16(bytes, 4);
    out.type_secondary = read_u16(bytes, 6);
    out.format_tag = bytes[8];
    out.header_word_at_22 = read_u32(bytes, 22);

    std::size_t cursor = 26;
    const bool compact_pb = (out.type_primary == 0x70 || out.type_primary == 0x71) &&
                            out.type_secondary == 0x62;
    if (!compact_pb) {
        if (bytes.size() < cursor + 4) return result;
        out.field6 = read_u16(bytes, cursor);
        out.field7 = read_u16(bytes, cursor + 2);
        cursor += 4;
        if (out.field7 != 0x02a4) {
            result.status = Module59HeaderStatus::invalid_directory_marker;
            return result;
        }
    }

    if (bytes.size() < cursor + 20) return result;
    for (std::size_t i = 0; i < out.fields8_to_11.size(); ++i) {
        out.fields8_to_11[i] = read_u16(bytes, cursor + i * 2);
    }
    cursor += 8;
    cursor += 4; // opaque 32-bit field consumed by the firmware
    out.field12 = read_u32(bytes, cursor);
    out.field14 = read_u32(bytes, cursor + 4);
    cursor += 8;
    out.bytes_consumed = cursor;

    const auto field11 = out.fields8_to_11[3];
    if (out.type_primary == 0x70 && out.type_secondary == 0x42) {
        if (field11 != 0x32) {
            result.status = Module59HeaderStatus::invalid_directory_marker;
            return result;
        }
    } else if (out.type_primary == 0x00f1) {
        if (out.type_secondary != 0x12 || field11 != 0x32) {
            result.status = Module59HeaderStatus::invalid_directory_marker;
            return result;
        }
    } else if ((out.type_primary == 0x70 || out.type_primary == 0x71) &&
               out.type_secondary == 0x62 && field11 != 0x2e) {
        result.status = Module59HeaderStatus::invalid_directory_marker;
        return result;
    }

    out.secondary_metadata_probe_required = out.type_primary != 0x75 &&
                                            out.type_primary != 0x76;
    result.status = Module59HeaderStatus::valid_prefix;
    return result;
}

} // namespace echo::storage
