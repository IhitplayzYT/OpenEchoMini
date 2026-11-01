#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace echo::storage {

struct Module38LineRecord {
    bool found_crlf{};
    std::uint32_t line_byte_length{};
    std::int32_t returned_length{};
    std::uint32_t trim_bytes{};
    std::vector<std::uint8_t> copied_bytes;
};

// Models FUN_0307b986's CRLF scan and line-class-specific trim. Byte-order
// conversion and the external copy helper are intentionally not reproduced.
[[nodiscard]] Module38LineRecord read_module38_line_record(
    std::span<const std::uint8_t> bounded_bytes,
    std::uint8_t line_class);

} // namespace echo::storage
