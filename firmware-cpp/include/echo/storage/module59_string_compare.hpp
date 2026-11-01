#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

struct Module59CompareOptions {
    bool skip_high_bytes{};
    bool skip_non_ascii_letters{};
    bool ascii_case_insensitive{};
    std::size_t compared_byte_limit{static_cast<std::size_t>(-1)};
};

struct Module59CompareResult {
    int order{};
    std::size_t equal_byte_count{};
    bool equal_count_initialized{};
    bool lhs_terminated{};
    bool rhs_terminated{};
};

// Compares NUL-terminated byte strings using the filter and ASCII-fold rules
// observed in FUN_0308ddc2. Missing terminators are treated as bounded NULs
// and reported in the result so the host implementation never reads past a
// supplied buffer.
[[nodiscard]] Module59CompareResult compare_module59_byte_strings(
    std::optional<std::span<const std::uint8_t>> lhs,
    std::optional<std::span<const std::uint8_t>> rhs,
    Module59CompareOptions options = {});

} // namespace echo::storage
