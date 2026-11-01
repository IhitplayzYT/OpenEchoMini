#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

inline constexpr std::size_t kModule80ApePredictorStateWords = 51;

// One sample update from FUN_010030b8. State is the 0xcc-byte predictor
// context; rolling_history is the per-chunk 51-word lookahead buffer plus
// decoded sample slots.
[[nodiscard]] std::optional<std::int32_t> module80_ape_predictor_step(
    std::span<std::int32_t, kModule80ApePredictorStateWords> state,
    std::span<std::int32_t> rolling_history,
    std::size_t sample_index,
    std::uint32_t channel,
    std::int32_t decoded_residual) noexcept;

// Reconstructs one FUN_01003508 chunk, including the rolling-window shift and
// final two-channel decorrelation. The input spans are overwritten in place.
[[nodiscard]] bool module80_ape_predictor_chunk(
    std::span<std::int32_t, kModule80ApePredictorStateWords> state,
    std::span<std::int32_t> channel_zero,
    std::span<std::int32_t> channel_one);

// Indices passed by FUN_01003508 for each channel's rolling-history taps.
struct Module80ApePredictorTapOffsets {
    std::uint8_t residual{};
    std::uint8_t cross_channel{};
    std::uint8_t residual_sign{};
    std::uint8_t cross_sign{};
};

[[nodiscard]] constexpr Module80ApePredictorTapOffsets
module80_ape_predictor_tap_offsets(std::uint32_t channel) noexcept {
    return channel == 0 ? Module80ApePredictorTapOffsets{10, 18, 42, 50}
                        : Module80ApePredictorTapOffsets{34, 26, 14, 5};
}

} // namespace echo::audio
