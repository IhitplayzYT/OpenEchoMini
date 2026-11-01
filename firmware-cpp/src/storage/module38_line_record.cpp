#include "echo/storage/module38_line_record.hpp"

namespace echo::storage {

Module38LineRecord read_module38_line_record(
    const std::span<const std::uint8_t> bounded_bytes,
    const std::uint8_t line_class) {
    Module38LineRecord result{};
    std::size_t end = 0;
    while (end + 1 < bounded_bytes.size()) {
        if (bounded_bytes[end] == '\r' && bounded_bytes[end + 1] == '\n') {
            result.found_crlf = true;
            break;
        }
        ++end;
    }
    if (!result.found_crlf) return result;

    result.line_byte_length = static_cast<std::uint32_t>(end);
    if (line_class == 4 || line_class == 5) {
        result.returned_length = static_cast<std::int32_t>(end);
        result.copied_bytes.assign(bounded_bytes.begin(), bounded_bytes.begin() + end);
        return result;
    }

    result.trim_bytes = line_class < 2 ? 6u : (line_class == 6 ? 4u : 1u);
    result.returned_length = static_cast<std::int32_t>(end) -
                             static_cast<std::int32_t>(result.trim_bytes);
    if (result.returned_length > 0) {
        result.copied_bytes.assign(
            bounded_bytes.begin(), bounded_bytes.begin() + result.returned_length);
    }
    return result;
}

} // namespace echo::storage
