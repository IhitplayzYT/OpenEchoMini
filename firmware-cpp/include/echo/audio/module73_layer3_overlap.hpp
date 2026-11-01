#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::audio {

// Models FUN_01005956's channel-strided accumulation and 18-value history
// advance. Returns false when the host spans cannot cover the observed writes.
[[nodiscard]] bool module73_layer3_overlap_and_output(
    std::span<const std::int32_t, 36> transformed_block,
    std::span<std::int32_t, 18> overlap_history,
    std::span<std::int32_t> interleaved_output,
    std::size_t channel_index) noexcept;

} // namespace echo::audio
