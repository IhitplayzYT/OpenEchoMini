#include "echo/image/jpeg_restart_marker.hpp"

#include <algorithm>

namespace echo::image {

Module19RestartResult module19_find_restart_marker(
    const std::span<const std::uint8_t> bytes,
    const std::uint8_t expected_restart_index) noexcept {
    constexpr std::size_t scan_limit = 0x600;
    const auto limit = std::min(bytes.size(), scan_limit);
    std::size_t cursor = 0;
    while (cursor < limit && bytes[cursor] != 0xffU) ++cursor;
    if (cursor == limit) {
        return {Module19RestartStatus::scan_limit, cursor,
                expected_restart_index};
    }

    while (cursor < limit && bytes[cursor] == 0xffU) ++cursor;
    if (cursor == limit) {
        return {Module19RestartStatus::scan_limit, cursor,
                expected_restart_index};
    }

    const auto expected_code = static_cast<std::uint8_t>(0xd0U + expected_restart_index);
    if (bytes[cursor] != expected_code) {
        return {Module19RestartStatus::marker_mismatch, cursor + 1U,
                expected_restart_index};
    }
    const auto next_index = static_cast<std::uint8_t>((expected_restart_index + 1U) & 7U);
    return {Module19RestartStatus::found, cursor + 1U, next_index};
}

} // namespace echo::image
