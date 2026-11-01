#include "echo/storage/module38_string_pair.hpp"

#include <algorithm>

namespace echo::storage {
namespace {

std::uint16_t read_u16(const std::span<const std::uint8_t> bytes,
                       const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(
               static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

} // namespace

Module38StringPairResult read_module38_string_pair(
    const std::span<const std::uint8_t> bounded_record) {
    Module38StringPairResult result{};
    if (bounded_record.size() < 10) {
        result.error = Module38StringPairError::truncated_header;
        return result;
    }

    result.first_length = read_u16(bounded_record, 0);
    result.second_length = read_u16(bounded_record, 2);
    std::size_t cursor = 10;
    result.consumed_bytes = 10;

    if (bounded_record.size() - cursor < result.first_length) {
        result.error = Module38StringPairError::truncated_first_value;
        return result;
    }
    const auto first_copy = std::min<std::size_t>(result.first_length, 0x80);
    result.first_copied.assign(bounded_record.begin() + cursor,
                               bounded_record.begin() + cursor + first_copy);
    cursor += result.first_length;
    result.consumed_bytes = static_cast<std::uint32_t>(cursor);

    if (bounded_record.size() - cursor < result.second_length) {
        result.error = Module38StringPairError::truncated_second_value;
        return result;
    }
    const auto second_copy = std::min<std::size_t>(result.second_length, 0x80);
    result.second_copied.assign(bounded_record.begin() + cursor,
                                bounded_record.begin() + cursor + second_copy);
    cursor += result.second_length;
    result.consumed_bytes = static_cast<std::uint32_t>(cursor);
    return result;
}

} // namespace echo::storage
