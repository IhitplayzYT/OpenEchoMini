#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <optional>
#include <span>

namespace echo::audio {

enum class Module74WmaParseStatus : std::uint8_t {
    ok = 0,
    malformed_or_unsupported = 1,
    invalid_context = 2,
    truncated = 3,
    invalid_asf_object = 4,
};

enum class Module74HeaderGuidRoute : std::uint8_t {
    reject_first_match,
    skip_unmatched_object,
    parse_codec_object,
};

// The firmware helper returns the first differing byte subtraction, or zero
// when all requested bytes match. The host wrapper reports short spans safely.
std::optional<int> compare_module74_bytes(
    std::span<const std::uint8_t> left,
    std::span<const std::uint8_t> right,
    std::size_t count) noexcept;

std::optional<Module74HeaderGuidRoute> route_module74_header_guid(
    std::span<const std::uint8_t> header_guid,
    std::span<const std::uint8_t> first_guid,
    std::span<const std::uint8_t> second_guid) noexcept;

enum class Module74OptionalValueSource : std::uint8_t {
    first_guid,
    second_guid,
};

struct Module74OptionalObjectPlan {
    Module74WmaParseStatus status{Module74WmaParseStatus::ok};
    Module74OptionalValueSource value_source{};
    std::uint8_t bytes_to_read{};
    bool present{};
};

struct Module74OuterHeader {
    std::array<std::uint8_t, 16> file_guid{};
    std::array<std::uint8_t, 16> optional_guid{};
    std::uint32_t codec_payload_size{};       // Header bytes 0x28..0x2b.
    std::uint32_t optional_presence_word{};   // Header bytes 0x2c..0x2f.
    std::uint32_t optional_payload_offset{};  // Header bytes 0x30..0x33.
    std::uint8_t state_flag_low7{};           // Low seven bits of byte 0x30.
};

enum class Module74OuterAction : std::uint8_t {
    reject_first_guid,
    skip_non_codec_object,
    skip_incomplete_object_count,
    parse_codec_object,
};

struct Module74OuterHeaderPlan {
    Module74WmaParseStatus status{Module74WmaParseStatus::ok};
    Module74OuterAction action{Module74OuterAction::skip_non_codec_object};
    std::uint16_t object_count_after{};
    std::uint8_t state_flag_low7{};
    std::uint32_t successful_cursor_advance{};
    Module74OuterHeader header{};
};

struct Module74AsfObjectHeader {
    std::array<std::uint8_t, 16> guid{};
    std::uint64_t object_size{};
};

struct Module74AsfDataObject {
    std::array<std::uint8_t, 16> guid{};
    std::uint32_t packet_count{};
    std::uint8_t marker_type{};
    std::uint8_t marker_version{};
};

struct Module74LengthField {
    std::uint32_t length{};
    std::size_t payload_offset{};
};

struct Module74LengthSequence {
    Module74WmaParseStatus status{Module74WmaParseStatus::ok};
    std::array<Module74LengthField, 3> bounded_fields{};
    std::uint32_t trailing_length{};
    std::size_t trailing_payload_offset{};
    std::size_t consumed_window{};
};

std::optional<Module74AsfObjectHeader> parse_module74_asf_object_header(
    std::span<const std::uint8_t> bytes,
    Module74WmaParseStatus& status) noexcept;

Module74WmaParseStatus parse_module74_asf_data_object(
    std::span<const std::uint8_t> bytes,
    std::span<const std::uint8_t> expected_guid,
    Module74AsfDataObject& output) noexcept;

Module74LengthSequence plan_module74_length_sequence(
    std::span<const std::uint8_t> bytes) noexcept;

std::optional<Module74OuterHeader> parse_module74_outer_header(
    std::span<const std::uint8_t> bytes) noexcept;

Module74OuterHeaderPlan plan_module74_outer_header(
    std::span<const std::uint8_t> header_bytes,
    std::span<const std::uint8_t> first_file_guid,
    std::span<const std::uint8_t> second_file_guid,
    std::uint16_t current_object_count,
    std::uint16_t expected_object_count,
    std::uint32_t available_window) noexcept;

// presence_word is the firmware's independent nonzero guard. payload_offset
// is the offset into the available input window used for the 8/9-byte read.
Module74OptionalObjectPlan plan_module74_optional_object(
    std::span<const std::uint8_t> object_guid,
    std::span<const std::uint8_t> first_guid,
    std::span<const std::uint8_t> second_guid,
    std::uint32_t presence_word,
    std::uint32_t payload_offset,
    std::uint32_t available_after_header) noexcept;

Module74WmaParseStatus decode_module74_optional_value(
    const Module74OptionalObjectPlan& plan,
    std::span<const std::uint8_t> payload,
    std::uint32_t& value) noexcept;

struct Module74WmaFormat {
    std::uint16_t codec_tag{};
    std::uint32_t samples_per_second{};
    std::uint32_t average_bytes_per_second{};
    std::uint16_t block_alignment{};
    std::uint16_t channels{};
    std::uint16_t bits_per_sample{};
    std::uint16_t valid_bits_per_sample{};
    std::uint32_t codec_specific_value{};
    std::uint16_t output_format{};
    std::uint16_t firmware_format_class{};
    std::uint16_t extension_word{};
    // Bytes written at firmware-state offsets 0x60..0x6f by tags 0x162/0x163.
    std::uint8_t extension_template[16]{};
};

// Parses the codec-format payload copied by module 74 after its 54-byte
// container header. The result deliberately exposes firmware fields instead
// of claiming a complete WAVEFORMATEX/ASF model.
Module74WmaParseStatus parse_module74_wma_codec_format(
    std::span<const std::uint8_t> payload,
    Module74WmaFormat& output) noexcept;

}  // namespace echo::audio
