#include "echo/audio/module63_compaction.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace echo::audio {
namespace {

void copy_sample(std::vector<std::uint8_t>& bytes, std::size_t destination,
                 std::size_t source, std::size_t width) {
    std::array<std::uint8_t, 4> sample{};
    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(source), width,
                sample.begin());
    std::copy_n(sample.begin(), width,
                bytes.begin() + static_cast<std::ptrdiff_t>(destination));
}

} // namespace

Module63CompactionResult compact_module63_capture(
    const Module63CompactionInput& input) {
    Module63CompactionResult result{};
    result.buffer.assign(input.buffer.begin(), input.buffer.end());
    result.sample_width = input.format_marker == 0x0f ? 2u : 4u;
    result.transfer_value = input.sample_count << 1;
    result.callback_count = result.transfer_value >> 2;

    const bool state_one = input.processing_state == 1;
    const bool state_two = input.processing_state == 2;
    const bool layout_three = input.layout_mode == 3;
    const bool layout_four = input.layout_mode == 4;
    if ((!state_one && !state_two) || (!layout_three && !layout_four)) {
        if ((state_one || state_two) && input.layout_mode != 3 && input.layout_mode != 4)
            result.outcome = Module63CompactionOutcome::invalid_mode_pair;
        return result;
    }

    const auto width = result.sample_width;
    const std::size_t iterations = input.sample_count >> 1;
    if (iterations > std::numeric_limits<std::size_t>::max() / (2 * width) ||
        result.buffer.size() < iterations * 2 * width) {
        result.outcome = Module63CompactionOutcome::insufficient_buffer;
        return result;
    }

    result.samples_processed = iterations;
    if (state_one) {
        result.transfer_value = static_cast<std::uint32_t>(iterations << 1);
        result.callback_count = result.transfer_value >> 2;
        result.outcome = layout_three
            ? Module63CompactionOutcome::compact_first_samples
            : Module63CompactionOutcome::select_second_samples;
        for (std::size_t i = 0; i < iterations; ++i) {
            const std::size_t source = (2 * i + (layout_four ? 1 : 0)) * width;
            copy_sample(result.buffer, i * width, source, width);
        }
    } else if (layout_three) {
        result.outcome = Module63CompactionOutcome::duplicate_first_samples;
        for (std::size_t i = 0; i < iterations; ++i) {
            const std::size_t source = i * 2 * width;
            copy_sample(result.buffer, source + width, source, width);
        }
    } else {
        result.outcome = Module63CompactionOutcome::select_second_samples;
        for (std::size_t i = 0; i < iterations; ++i) {
            const std::size_t source = (2 * i + 1) * width;
            copy_sample(result.buffer, i * 2 * width, source, width);
        }
    }
    return result;
}

} // namespace echo::audio
