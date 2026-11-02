#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct Module78FlacLpcDspPlan {
    std::uint32_t block_size{};
    std::uint32_t predictor_order{};
    std::uint32_t shift{};
    std::uint32_t initial_control_word{};
    std::uint32_t submit_control_word{};
    std::uint32_t predictor_parameters{};
    std::uint32_t coefficient_count{};
    std::uint32_t start_flag{0x8000};
    std::uint32_t completion_mask{0x8000};
};

// Describes the higher-order LPC DSP setup seen in FUN_0100423e. It does not
// access hardware or reproduce the DSP's sample arithmetic.
[[nodiscard]] inline std::optional<Module78FlacLpcDspPlan>
plan_module78_flac_lpc_dsp(std::uint32_t block_size,
                           std::uint32_t predictor_order,
                           std::uint32_t shift,
                           std::span<const std::int32_t> coefficients) noexcept {
    if (block_size == 0 || block_size > 0x1200 || predictor_order <= 2 ||
        predictor_order > 32 || shift > 31 ||
        coefficients.size() != predictor_order) {
        return std::nullopt;
    }
    const std::uint32_t control = 0xa0000000U | (block_size << 15);
    return Module78FlacLpcDspPlan{
        block_size,
        predictor_order,
        shift,
        control,
        control | 3U,
        (predictor_order << 16) | (shift << 8),
        predictor_order,
    };
}

} // namespace echo::audio
