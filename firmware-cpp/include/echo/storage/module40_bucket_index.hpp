#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

// Returns the first threshold index whose value is >= normalized_key, matching
// module 40's forward scan. The caller supplies the table and key mapping.
[[nodiscard]] std::size_t select_module40_bucket(
    std::span<const std::uint16_t> thresholds,
    std::uint16_t normalized_key) noexcept;

} // namespace echo::storage
