#pragma once

#include <cstdint>

namespace echo::audio {

enum class Module63BlockCountOutcome : std::uint8_t {
    reject_zero_count,
    quotient_ready,
    invalid_zero_divisor,
};

struct Module63BlockCountInput {
    std::uint32_t requested_count{};
    std::uint32_t mode{};
    std::uint32_t unit_size{};
};

struct Module63BlockCountPlan {
    Module63BlockCountOutcome outcome{Module63BlockCountOutcome::reject_zero_count};
    bool requested_count_written{};
    std::uint32_t requested_count_after{};
    bool sentinel_written{};
    std::uint32_t sentinel_after{};
    std::uint32_t numerator{};
    bool block_count_written{};
    std::uint32_t block_count_after{};
    bool error_callback_requested{};
    std::uint8_t error_code{};
};

// Plans FUN_03080962's count update without invoking its error callback.
[[nodiscard]] Module63BlockCountPlan plan_module63_block_count(
    const Module63BlockCountInput& input) noexcept;

} // namespace echo::audio
