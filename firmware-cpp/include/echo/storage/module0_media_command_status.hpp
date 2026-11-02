#pragma once

#include <cstdint>

namespace echo::storage {

struct Module0MediaCommandStatusInput {
    std::uint32_t command_flags{};
    std::uint32_t command_status_word{};
    std::int32_t response_fifo_status{};
    std::uint32_t completion_status_word{};
    std::uint32_t completion_threshold{};
    bool post_command_wait_timed_out{};
    bool response_buffer_available{};
};

struct Module0MediaCommandStatusPlan {
    std::int32_t result_code{};
    std::uint8_t response_words_to_copy{};
    bool clear_command_status_word{true};
};

// Models FUN_0305f28a's observed command-status mapping after its command
// polling stage. Timing, registers, FIFO access, and diagnostic output remain
// caller-owned.
[[nodiscard]] inline Module0MediaCommandStatusPlan plan_module0_media_command_status(
    Module0MediaCommandStatusInput input) noexcept {
    const auto flags = input.command_flags;
    const auto status = input.command_status_word;
    std::int32_t result = 0;

    if ((status & 0x100U) != 0) {
        result = 0x20;
    } else if ((flags & 0x200U) != 0) {
        const bool single_block_mode = (flags & 0xe00U) == 0x600U;
        if (single_block_mode) {
            if ((status & 0x80U) != 0) result = 0x40;
            else if ((status & 0x8000U) != 0) result = 0x100;
        } else if ((status & 0xa280U) != 0) {
            if ((status & 0x2000U) != 0) result = 0x200;
            else if ((status & 0x8000U) != 0) {
                result = (flags & 0x3fU) != 0x0eU
                    ? 0x100 : input.response_fifo_status;
            } else if ((status & 0x200U) != 0) result = 0x80;
            else if ((status & 0x80U) != 0) result = 0x40;
            else result = input.response_fifo_status;
        } else {
            result = input.response_fifo_status;
        }
    }

    if (result != 0) return {result, 0, true};
    if ((flags & 0x10000U) != 0 && input.post_command_wait_timed_out) {
        return {0x400, 0, true};
    }

    std::uint8_t response_words = 0;
    if ((flags & 0x40U) != 0 && input.response_buffer_available) {
        response_words = 1;
        if ((flags & 0x80U) != 0) response_words = 4;
    }
    if (input.completion_threshold < 0x191U) return {0, response_words, true};
    if ((input.completion_status_word & 0x40U) != 0) return {0x10, response_words, true};
    if ((input.completion_status_word & 0x2U) != 0) return {8, response_words, true};
    return {0, response_words, true};
}

} // namespace echo::storage
