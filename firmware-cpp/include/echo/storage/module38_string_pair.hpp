#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace echo::storage {

enum class Module38StringPairError : std::uint8_t {
    none,
    truncated_header,
    truncated_first_value,
    truncated_second_value,
};

struct Module38StringPairResult {
    std::uint16_t first_length{};
    std::uint16_t second_length{};
    std::vector<std::uint8_t> first_copied;
    std::vector<std::uint8_t> second_copied;
    std::uint32_t consumed_bytes{};
    Module38StringPairError error{Module38StringPairError::none};

    [[nodiscard]] bool complete() const noexcept {
        return error == Module38StringPairError::none;
    }
};

// Models FUN_0307c30c: two lengths, six opaque header bytes, then two values.
[[nodiscard]] Module38StringPairResult read_module38_string_pair(
    std::span<const std::uint8_t> bounded_record);

} // namespace echo::storage
