#pragma once

#include <cstdint>

namespace echo::storage {

struct Module0MediaFifoPlan {
    std::uint32_t requested_bytes{};
    std::uint32_t full_words{};
    std::uint32_t total_words{};
    std::uint8_t trailing_bytes{};
    std::uint16_t fifo_error_bit{0x0004};
    std::uint16_t trailing_word_error_code{0x1000};
    bool main_loop_stops_when_fifo_error_set{true};
    bool trailing_word_checks_fifo_error{true};
};

// Describes the word/tail layout consumed by FUN_0305f5e2. This planner does
// not read FIFO registers or copy bytes into a destination buffer.
[[nodiscard]] inline Module0MediaFifoPlan plan_module0_media_fifo_receive(
    std::uint32_t requested_bytes) noexcept {
    const auto full_words = requested_bytes / 4U;
    const auto trailing = static_cast<std::uint8_t>(requested_bytes % 4U);
    return {requested_bytes, full_words,
            full_words + (trailing == 0 ? 0U : 1U), trailing,
            0x0004, 0x1000, true, true};
}

} // namespace echo::storage
