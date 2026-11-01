#include "echo/audio/module74_wma_format.hpp"

#include <cstddef>

namespace echo::audio {
namespace {

std::uint16_t u16le(std::span<const std::uint8_t> bytes,
                    std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(bytes[offset + 1]) << 8;
}

std::uint32_t u32le(std::span<const std::uint8_t> bytes,
                    std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset]) |
           static_cast<std::uint32_t>(bytes[offset + 1]) << 8 |
           static_cast<std::uint32_t>(bytes[offset + 2]) << 16 |
           static_cast<std::uint32_t>(bytes[offset + 3]) << 24;
}

std::uint64_t u64le(std::span<const std::uint8_t> bytes,
                    std::size_t offset) noexcept {
    return static_cast<std::uint64_t>(u32le(bytes, offset)) |
           static_cast<std::uint64_t>(u32le(bytes, offset + 4)) << 32;
}

bool assign_output_format(std::uint16_t channels,
                          std::uint16_t& output_format) noexcept {
    switch (channels) {
        case 1: output_format = 4; return true;
        case 2: output_format = 3; return true;
        case 6: output_format = 0x3f; return true;
        default: return false;
    }
}

}  // namespace

std::optional<int> compare_module74_bytes(
    std::span<const std::uint8_t> left,
    std::span<const std::uint8_t> right,
    std::size_t count) noexcept {
    if (left.size() < count || right.size() < count) return std::nullopt;
    for (std::size_t i = 0; i < count; ++i) {
        if (left[i] != right[i])
            return static_cast<int>(left[i]) - static_cast<int>(right[i]);
    }
    return 0;
}

std::optional<Module74HeaderGuidRoute> route_module74_header_guid(
    std::span<const std::uint8_t> header_guid,
    std::span<const std::uint8_t> first_guid,
    std::span<const std::uint8_t> second_guid) noexcept {
    const auto first_match = compare_module74_bytes(header_guid, first_guid, 16);
    if (!first_match) return std::nullopt;
    if (*first_match == 0)
        return Module74HeaderGuidRoute::reject_first_match;

    const auto second_match = compare_module74_bytes(header_guid, second_guid, 16);
    if (!second_match) return std::nullopt;
    if (*second_match != 0)
        return Module74HeaderGuidRoute::skip_unmatched_object;
    return Module74HeaderGuidRoute::parse_codec_object;
}

std::optional<Module74OuterHeader> parse_module74_outer_header(
    std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < 0x36) return std::nullopt;
    Module74OuterHeader header{};
    for (std::size_t i = 0; i < header.file_guid.size(); ++i) {
        header.file_guid[i] = bytes[i];
        header.optional_guid[i] = bytes[0x10 + i];
    }
    header.codec_payload_size = u32le(bytes, 0x28);
    header.optional_presence_word = u32le(bytes, 0x2c);
    header.optional_payload_offset = u32le(bytes, 0x30);
    header.state_flag_low7 = static_cast<std::uint8_t>(bytes[0x30] & 0x7f);
    return header;
}

std::optional<Module74AsfObjectHeader> parse_module74_asf_object_header(
    std::span<const std::uint8_t> bytes,
    Module74WmaParseStatus& status) noexcept {
    if (bytes.size() < 0x18) {
        status = Module74WmaParseStatus::truncated;
        return std::nullopt;
    }
    Module74AsfObjectHeader header{};
    for (std::size_t i = 0; i < header.guid.size(); ++i)
        header.guid[i] = bytes[i];
    header.object_size = u64le(bytes, 0x10);
    if (static_cast<std::uint32_t>(header.object_size) < 0x18) {
        status = Module74WmaParseStatus::invalid_asf_object;
        return std::nullopt;
    }
    status = Module74WmaParseStatus::ok;
    return header;
}

Module74WmaParseStatus parse_module74_asf_data_object(
    std::span<const std::uint8_t> bytes,
    std::span<const std::uint8_t> expected_guid,
    Module74AsfDataObject& output) noexcept {
    if (bytes.size() < 0x1e) return Module74WmaParseStatus::truncated;
    const auto match = compare_module74_bytes(bytes, expected_guid, 16);
    if (!match || *match != 0 || bytes[0x1c] != 1 || bytes[0x1d] != 2)
        return Module74WmaParseStatus::invalid_asf_object;

    Module74AsfDataObject parsed{};
    for (std::size_t i = 0; i < parsed.guid.size(); ++i)
        parsed.guid[i] = bytes[i];
    parsed.packet_count = u32le(bytes, 0x10);
    parsed.marker_type = bytes[0x1c];
    parsed.marker_version = bytes[0x1d];
    output = parsed;
    return Module74WmaParseStatus::ok;
}

Module74LengthSequence plan_module74_length_sequence(
    std::span<const std::uint8_t> bytes) noexcept {
    Module74LengthSequence plan{};
    constexpr std::array<std::uint32_t, 3> maxima{0x20, 0x10, 0x20};
    std::size_t cursor = 0;

    for (std::size_t i = 0; i < plan.bounded_fields.size(); ++i) {
        if (bytes.size() - cursor < 4) {
            plan.status = Module74WmaParseStatus::truncated;
            return plan;
        }
        const auto length = u32le(bytes, cursor);
        cursor += 4;
        if (length > maxima[i] || length > bytes.size() - cursor) {
            plan.status = Module74WmaParseStatus::truncated;
            return plan;
        }
        plan.bounded_fields[i] = {length, cursor};
        cursor += length;
    }

    if (bytes.size() - cursor < 4) {
        plan.status = Module74WmaParseStatus::truncated;
        return plan;
    }
    plan.trailing_length = u32le(bytes, cursor);
    cursor += 4;
    if (plan.trailing_length > bytes.size() - cursor) {
        plan.status = Module74WmaParseStatus::truncated;
        return plan;
    }
    plan.trailing_payload_offset = cursor;
    plan.consumed_window = bytes.size();
    return plan;
}

Module74OuterHeaderPlan plan_module74_outer_header(
    std::span<const std::uint8_t> header_bytes,
    std::span<const std::uint8_t> first_file_guid,
    std::span<const std::uint8_t> second_file_guid,
    std::uint16_t current_object_count,
    std::uint16_t expected_object_count,
    std::uint32_t available_window) noexcept {
    Module74OuterHeaderPlan plan{};
    if (available_window < 0x36 || header_bytes.size() < 0x36) {
        plan.status = Module74WmaParseStatus::truncated;
        return plan;
    }

    const auto header = parse_module74_outer_header(header_bytes);
    if (!header) {
        plan.status = Module74WmaParseStatus::truncated;
        return plan;
    }
    plan.header = *header;

    const auto route = route_module74_header_guid(
        plan.header.file_guid, first_file_guid, second_file_guid);
    if (!route) {
        plan.status = Module74WmaParseStatus::malformed_or_unsupported;
        return plan;
    }
    if (*route == Module74HeaderGuidRoute::reject_first_match) {
        plan.action = Module74OuterAction::reject_first_guid;
        plan.status = Module74WmaParseStatus::malformed_or_unsupported;
        return plan;
    }
    if (*route == Module74HeaderGuidRoute::skip_unmatched_object) {
        plan.action = Module74OuterAction::skip_non_codec_object;
        plan.successful_cursor_advance = available_window;
        return plan;
    }

    plan.object_count_after = static_cast<std::uint16_t>(current_object_count + 1u);
    if (plan.object_count_after != expected_object_count) {
        plan.action = Module74OuterAction::skip_incomplete_object_count;
        plan.successful_cursor_advance = available_window;
        return plan;
    }

    const auto required = static_cast<std::uint64_t>(0x36) +
                          plan.header.codec_payload_size;
    if (required > available_window) {
        plan.status = Module74WmaParseStatus::truncated;
        return plan;
    }
    plan.action = Module74OuterAction::parse_codec_object;
    plan.state_flag_low7 = plan.header.state_flag_low7;
    plan.successful_cursor_advance = available_window;
    return plan;
}

Module74OptionalObjectPlan plan_module74_optional_object(
    std::span<const std::uint8_t> object_guid,
    std::span<const std::uint8_t> first_guid,
    std::span<const std::uint8_t> second_guid,
    std::uint32_t presence_word,
    std::uint32_t payload_offset,
    std::uint32_t available_after_header) noexcept {
    Module74OptionalObjectPlan plan{};
    if (presence_word == 0) return plan;

    const auto first_match = compare_module74_bytes(object_guid, first_guid, 16);
    if (!first_match) {
        plan.status = Module74WmaParseStatus::malformed_or_unsupported;
        return plan;
    }
    if (*first_match == 0) {
        plan.value_source = Module74OptionalValueSource::first_guid;
        plan.bytes_to_read = 9;
    } else {
        const auto second_match = compare_module74_bytes(object_guid, second_guid, 16);
        if (!second_match || *second_match != 0) {
            plan.status = Module74WmaParseStatus::malformed_or_unsupported;
            return plan;
        }
        plan.value_source = Module74OptionalValueSource::second_guid;
        plan.bytes_to_read = 8;
    }

    const auto required = static_cast<std::uint64_t>(payload_offset) + plan.bytes_to_read;
    if (required > available_after_header) {
        plan.status = Module74WmaParseStatus::truncated;
        return plan;
    }
    plan.present = true;
    return plan;
}

Module74WmaParseStatus decode_module74_optional_value(
    const Module74OptionalObjectPlan& plan,
    std::span<const std::uint8_t> payload,
    std::uint32_t& value) noexcept {
    if (plan.status != Module74WmaParseStatus::ok) return plan.status;
    if (!plan.present) return Module74WmaParseStatus::ok;
    if (payload.size() < plan.bytes_to_read) return Module74WmaParseStatus::truncated;

    if (plan.value_source == Module74OptionalValueSource::first_guid) {
        value = u32le(payload, 0);
        return Module74WmaParseStatus::ok;
    }
    if (payload[0] > 1) return Module74WmaParseStatus::malformed_or_unsupported;
    value = static_cast<std::uint32_t>(payload[0]) * payload[1];
    return Module74WmaParseStatus::ok;
}

Module74WmaParseStatus parse_module74_wma_codec_format(
    std::span<const std::uint8_t> payload,
    Module74WmaFormat& output) noexcept {
    if (payload.size() < 2)
        return Module74WmaParseStatus::malformed_or_unsupported;

    Module74WmaFormat parsed{};
    parsed.codec_tag = u16le(payload, 0);
    std::size_t minimum_size = 0;
    switch (parsed.codec_tag) {
        case 0x0160: minimum_size = 0x16; parsed.firmware_format_class = 1; break;
        case 0x0161: minimum_size = 0x1c; parsed.firmware_format_class = 2; break;
        case 0x0162:
        case 0x0163: minimum_size = 0x24; parsed.firmware_format_class = 3; break;
        default: return Module74WmaParseStatus::malformed_or_unsupported;
    }
    if (payload.size() < minimum_size) return Module74WmaParseStatus::malformed_or_unsupported;

    parsed.channels = u16le(payload, 2);
    parsed.samples_per_second = u32le(payload, 4);
    parsed.average_bytes_per_second = u32le(payload, 8);
    parsed.block_alignment = u16le(payload, 12);

    if (parsed.codec_tag == 0x0160 || parsed.codec_tag == 0x0161) {
        parsed.bits_per_sample = u16le(payload, 14);
        parsed.valid_bits_per_sample = parsed.bits_per_sample;
        parsed.codec_specific_value = parsed.codec_tag == 0x0160
                                          ? u16le(payload, 18)
                                          : u32le(payload, 18);
        parsed.extension_word = u16le(payload,
                                      parsed.codec_tag == 0x0160 ? 20 : 22);

        if (!assign_output_format(parsed.channels, parsed.output_format))
            return Module74WmaParseStatus::malformed_or_unsupported;
        if (parsed.codec_tag == 0x0160 && parsed.channels == 6)
            return Module74WmaParseStatus::malformed_or_unsupported;
        if (parsed.codec_tag == 0x0161 && parsed.channels != 1 &&
            parsed.channels != 2 && parsed.channels != 6)
            return Module74WmaParseStatus::malformed_or_unsupported;
        output = parsed;
        return Module74WmaParseStatus::ok;
    }

    parsed.valid_bits_per_sample = u16le(payload, 14);
    parsed.bits_per_sample = static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(parsed.valid_bits_per_sample) + 7u) & ~7u);
    parsed.codec_specific_value = u32le(payload, 20);
    parsed.extension_word = u16le(payload, 32);
    parsed.extension_template[0] = 1;
    parsed.extension_template[6] = 0x10;
    parsed.extension_template[8] = 0x80;
    parsed.extension_template[11] = 0xaa;
    parsed.extension_template[13] = 0x38;
    parsed.extension_template[14] = 0x9b;
    parsed.extension_template[15] = 0x71;
    output = parsed;
    return Module74WmaParseStatus::ok;
}

}  // namespace echo::audio
