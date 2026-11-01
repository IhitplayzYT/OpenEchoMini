#include "echo/audio/module33_fixed_point_filter.hpp"

#include <bit>
#include <algorithm>

namespace echo::audio {
namespace {

using Wide = Module33FixedState;

Wide add(const Wide left, const Wide right) noexcept {
    const auto low_sum = static_cast<std::uint64_t>(left.low) + right.low;
    const auto high_sum = static_cast<std::uint32_t>(left.high) +
                          static_cast<std::uint32_t>(right.high) +
                          static_cast<std::uint32_t>(low_sum >> 32);
    return {std::bit_cast<std::int32_t>(high_sum),
            static_cast<std::uint32_t>(low_sum)};
}

Wide multiply_q13(const Wide value, const std::int32_t coefficient) noexcept {
    // The low-limb product fits signed 64-bit: uint32 * a 16-bit coefficient.
    // Combining its upper word with high*coefficient yields the low 64 bits of
    // the full product, matching UMULL/MLA in the firmware.
    const auto low_product = static_cast<std::int64_t>(value.low) * coefficient;
    const auto product_high = static_cast<std::uint32_t>(
        static_cast<std::int64_t>(value.high) * coefficient +
        (low_product >> 32));
    const auto product_low = static_cast<std::uint32_t>(low_product);

    const auto shifted_low = (product_low >> 13) | (product_high << 19);
    const auto signed_high = std::bit_cast<std::int32_t>(product_high);
    const auto shifted_high = static_cast<std::uint32_t>(signed_high >> 13);
    return {std::bit_cast<std::int32_t>(shifted_high), shifted_low};
}

Wide sample_drive(const std::int16_t sample,
                  const Module33ChannelState& state) noexcept {
    // Q13 * (x[n] - 2*x[n-1] + x[n-2]).
    const auto drive = static_cast<std::int64_t>(sample) * 0x2000 -
                       static_cast<std::int64_t>(state.previous_input) * 0x4000 +
                       static_cast<std::int64_t>(state.input_before_previous) * 0x2000;
    return {drive < 0 ? -1 : 0, static_cast<std::uint32_t>(drive)};
}

std::int16_t saturate_q13(const Wide value) noexcept {
    const auto shifted_low = (value.low >> 13) |
                             (static_cast<std::uint32_t>(value.high) << 19);
    const auto shifted_high = static_cast<std::int32_t>(value.high >> 13);
    const auto sample = std::bit_cast<std::int32_t>(shifted_low);

    if (shifted_high == 0 && sample >= 0 && sample <= 0x7fff) {
        return static_cast<std::int16_t>(sample);
    }
    if (shifted_high == -1 && sample < 0 && sample >= -0x8000) {
        return static_cast<std::int16_t>(sample);
    }
    return shifted_high < 0 ? static_cast<std::int16_t>(-0x8000)
                            : static_cast<std::int16_t>(0x7fff);
}

void process_channel(std::span<std::int16_t> pcm,
                     const std::size_t offset,
                     const std::size_t frames,
                     Module33ChannelState& state) noexcept {
    for (std::size_t frame = 0; frame < frames; ++frame) {
        auto& sample = pcm[offset + frame * 2];
        const auto input = sample;
        const auto driven = sample_drive(input, state);
        const auto first = add(add(multiply_q13(state.first_history, 0x3fef),
                                   multiply_q13(state.second_history, -0x1fef)),
                               driven);
        sample = saturate_q13(first);

        state.second_history = state.first_history;
        state.first_history = first;
        state.input_before_previous = state.previous_input;
        state.previous_input = input;
    }
}

} // namespace

std::size_t process_module33_fixed_point_filter(
    const std::span<std::int16_t> interleaved_pcm,
    const std::size_t frame_count,
    Module33StereoState& state) noexcept {
    const auto available_frames = interleaved_pcm.size() / 2;
    const auto frames = std::min(frame_count, available_frames);
    if (frames == 0) {
        return 0;
    }
    process_channel(interleaved_pcm, 0, frames, state.channels[0]);
    process_channel(interleaved_pcm, 1, frames, state.channels[1]);
    return frames;
}

} // namespace echo::audio
