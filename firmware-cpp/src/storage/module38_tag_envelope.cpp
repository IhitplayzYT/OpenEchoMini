#include "echo/storage/module38_tag_envelope.hpp"

#include <algorithm>
#include <utility>

namespace echo::storage {
namespace {

std::uint16_t read_u16(const std::span<const std::uint8_t> bytes,
                       const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(
               static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

} // namespace

Module38TagParseResult parse_module38_tag_envelopes(
    const std::span<const std::uint8_t> bounded_record_area) {
    Module38TagParseResult result{};
    if (bounded_record_area.size() < 2) {
        result.error = Module38TagParseError::truncated_count;
        return result;
    }

    result.declared_count = static_cast<std::int16_t>(read_u16(bounded_record_area, 0));
    result.consumed_bytes = 2;
    if (result.declared_count <= 0) return result;

    const auto max_records = std::min<std::size_t>(
        static_cast<std::size_t>(result.declared_count),
        (bounded_record_area.size() - 2) / 6);
    result.records.reserve(max_records);

    for (std::int32_t index = 0; index < result.declared_count; ++index) {
        auto cursor = result.consumed_bytes;
        if (bounded_record_area.size() - cursor < 2) {
            result.error = Module38TagParseError::truncated_signature_length;
            return result;
        }

        Module38TagEnvelope record{};
        const auto raw_signature_length = read_u16(bounded_record_area, cursor);
        const auto signature_length = static_cast<std::int16_t>(raw_signature_length);
        cursor += 2;
        if (signature_length < 0) {
            result.error = Module38TagParseError::negative_signature_length;
            return result;
        }
        record.signature_length = static_cast<std::uint16_t>(signature_length);
        if (bounded_record_area.size() - cursor < record.signature_length) {
            result.error = Module38TagParseError::truncated_signature;
            return result;
        }
        record.signature.assign(bounded_record_area.begin() + cursor,
                                bounded_record_area.begin() + cursor + record.signature_length);
        cursor += record.signature_length;
        if (bounded_record_area.size() - cursor < 4) {
            result.error = Module38TagParseError::truncated_value_header;
            return result;
        }

        record.value_kind = static_cast<std::int16_t>(read_u16(bounded_record_area, cursor));
        record.value_length = read_u16(bounded_record_area, cursor + 2);
        cursor += 4;
        if (bounded_record_area.size() - cursor < record.value_length) {
            result.error = Module38TagParseError::truncated_value;
            return result;
        }
        record.value.assign(bounded_record_area.begin() + cursor,
                            bounded_record_area.begin() + cursor + record.value_length);
        cursor += record.value_length;
        result.records.push_back(std::move(record));
        result.consumed_bytes = cursor;
    }
    return result;
}

} // namespace echo::storage
