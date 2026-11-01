#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::image {

enum class Module19RestartStatus {
    found,
    marker_mismatch,
    scan_limit,
};

struct Module19RestartResult {
    Module19RestartStatus status{};
    std::size_t bytes_consumed{};
    std::uint8_t next_restart_index{};
};

// Models module 19's bounded search for FF D0..D7, including repeated FF fill
// bytes. The firmware allows at most 0x600 byte reads per restart boundary.
[[nodiscard]] Module19RestartResult module19_find_restart_marker(
    std::span<const std::uint8_t> bytes,
    std::uint8_t expected_restart_index) noexcept;

} // namespace echo::image
