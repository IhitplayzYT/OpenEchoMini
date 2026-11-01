#pragma once

#include <cstdint>

namespace echo::apps {

struct Module17ProgressBandRecord {
    std::uint32_t start{};
    std::uint32_t current{};
    std::uint32_t origin{};
    std::uint32_t span{};
    std::uint32_t record_selector{};
};

struct Module17ProgressBandInput {
    bool event_16e{};
    bool event_16b{};
    bool event_16d{};
    Module17ProgressBandRecord band_16b{};
    Module17ProgressBandRecord band_16d{};
    std::uint32_t draw_base{};
    std::uint32_t shared_comparison_value{};
    std::uint32_t alternate_width{};
};

struct Module17ProgressBandDraw {
    bool emitted{};
    std::uint32_t event_id{};
    std::uint32_t base{};
    std::uint16_t position{};
    std::uint16_t width{};
    std::uint32_t mode{};
};

// Models FUN_0307b9ee's event priority, unsigned span check and arguments to
// the external draw routine at 0x03064d08. Shared record fields remain raw.
[[nodiscard]] Module17ProgressBandDraw plan_module17_progress_band(
    const Module17ProgressBandInput& input) noexcept;

} // namespace echo::apps
