#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace echo::audio {

enum class Module63CompactionOutcome : std::uint8_t {
    unchanged,
    compact_first_samples,
    select_second_samples,
    duplicate_first_samples,
    invalid_mode_pair,
    insufficient_buffer,
};

struct Module63CompactionInput {
    std::span<const std::uint8_t> buffer;
    std::uint32_t sample_count{};
    std::uint32_t processing_state{};
    std::uint32_t layout_mode{};
    std::uint32_t format_marker{};
};

struct Module63CompactionResult {
    Module63CompactionOutcome outcome{Module63CompactionOutcome::unchanged};
    std::vector<std::uint8_t> buffer;
    std::uint32_t transfer_value{};
    std::uint32_t callback_count{};
    std::size_t sample_width{};
    std::size_t samples_processed{};
};

// Replays only FUN_03080b02's in-place sample copies. transfer_value is the
// observed r6 value; callback_count is the value passed as r3 (r6 >> 2).
[[nodiscard]] Module63CompactionResult compact_module63_capture(
    const Module63CompactionInput& input);

} // namespace echo::audio
