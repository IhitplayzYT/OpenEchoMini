#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace echo::storage {

struct Module38TagEnvelope {
    std::uint16_t signature_length{};
    std::vector<std::uint8_t> signature;
    std::int16_t value_kind{};
    std::uint16_t value_length{};
    std::vector<std::uint8_t> value;
};

enum class Module38TagParseError : std::uint8_t {
    none,
    truncated_count,
    truncated_signature_length,
    negative_signature_length,
    truncated_signature,
    truncated_value_header,
    truncated_value,
};

struct Module38TagParseResult {
    std::int16_t declared_count{};
    std::vector<Module38TagEnvelope> records;
    std::size_t consumed_bytes{};
    Module38TagParseError error{Module38TagParseError::none};

    [[nodiscard]] bool complete() const noexcept {
        return error == Module38TagParseError::none;
    }
};

// Reads the bounded envelope sequence observed in FUN_0307bf26. It preserves
// raw signatures and values without assigning meanings to numeric tags.
[[nodiscard]] Module38TagParseResult parse_module38_tag_envelopes(
    std::span<const std::uint8_t> bounded_record_area);

} // namespace echo::storage
