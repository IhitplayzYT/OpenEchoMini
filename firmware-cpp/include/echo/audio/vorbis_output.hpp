#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace echo::audio {

struct VorbisOutputReadPlan {
    std::uint32_t sample_frames{};
    std::uint32_t pcm_bytes{};
    bool render_pcm{};
};

struct VorbisOutputReadCommit {
    std::uint32_t sample_frames{};
    std::uint32_t pcm_bytes{};
    std::uint64_t consumed_samples{};
    std::optional<std::uint32_t> output_item_index;
};

struct VorbisOverlapRegionPlan {
    std::uint32_t selected_half_block{};
    std::uint32_t spectrum_quarter_offset{};
    std::int32_t unwindowed_prefix_frames{};
    std::int32_t alternate_window_frames{};
    bool use_second_window{};
};

// FUN_01004bbc lays out fixed per-channel working stores used by FUN_0100ada2.
inline constexpr std::size_t kVorbisFirstSpectrumCapacity = 0x1000;
inline constexpr std::size_t kVorbisSecondSpectrumCapacity = 0x0800;

struct VorbisOverlapChannelBuffers {
    std::span<const std::int32_t> first_spectrum;
    std::span<const std::int32_t> second_spectrum;
};

// Resolves FUN_0100ada2's four previous/next window-flag branches. The frame
// regions are signed because adjacent blocks can grow or shrink.
[[nodiscard]] inline std::optional<VorbisOverlapRegionPlan>
plan_vorbis_overlap_regions(
    std::uint32_t first_block_size,
    std::uint32_t second_block_size,
    bool first_window_long,
    bool second_window_long) noexcept {
    if (first_block_size < 64 || second_block_size < 64 ||
        first_block_size > 8192 || second_block_size > 8192 ||
        first_block_size % 4 != 0 || second_block_size % 4 != 0 ||
        (first_block_size & (first_block_size - 1)) != 0 ||
        (second_block_size & (second_block_size - 1)) != 0) {
        return std::nullopt;
    }

    const auto first_quarter = static_cast<std::int32_t>(first_block_size / 4);
    const auto second_quarter = static_cast<std::int32_t>(second_block_size / 4);
    const auto transition = second_quarter - first_quarter;
    const bool both_long = first_window_long && second_window_long;
    const bool copy_prefix = first_window_long && !second_window_long;
    const bool alternate_window = !first_window_long && second_window_long;
    return VorbisOverlapRegionPlan{
        both_long ? second_block_size / 2 : first_block_size / 2,
        first_window_long ? second_block_size / 4 : first_block_size / 4,
        copy_prefix ? transition : 0,
        alternate_window ? transition : 0,
        both_long};
}

[[nodiscard]] inline std::int32_t vorbis_output_wrap32(std::int64_t value) noexcept {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
}

// FUN_0100ada2 converts Q9 output to signed 16-bit PCM with saturation.
[[nodiscard]] inline std::int16_t vorbis_output_q9_to_s16(std::int32_t value) noexcept {
    const auto scaled = value >> 9;
    if (scaled > std::numeric_limits<std::int16_t>::max()) {
        return std::numeric_limits<std::int16_t>::max();
    }
    if (scaled < std::numeric_limits<std::int16_t>::min()) {
        return std::numeric_limits<std::int16_t>::min();
    }
    return static_cast<std::int16_t>(scaled);
}

// Reproduces the renderer's two-term Q8-window multiply followed by Q9 shift.
[[nodiscard]] inline std::int16_t mix_vorbis_windowed_s16(
    std::int32_t left,
    std::uint8_t left_window,
    std::int32_t right,
    std::uint8_t right_window,
    bool subtract_right = false) noexcept {
    const auto left_product = vorbis_output_wrap32(
        static_cast<std::int64_t>(left >> 8) * left_window);
    const auto right_product = vorbis_output_wrap32(
        static_cast<std::int64_t>(right >> 8) * right_window);
    const auto combined = subtract_right
        ? vorbis_output_wrap32(static_cast<std::int64_t>(left_product) - right_product)
        : vorbis_output_wrap32(static_cast<std::int64_t>(left_product) + right_product);
    return vorbis_output_q9_to_s16(combined);
}

// Ports the renderer's unwindowed transition-prefix copy. The source cursor
// points just past the selected spectrum range; output is channel-strided.
[[nodiscard]] inline bool copy_vorbis_overlap_prefix(
    std::span<const std::int32_t> spectrum,
    std::size_t source_end,
    std::uint32_t requested_begin,
    std::uint32_t requested_end,
    std::uint32_t prefix_frames,
    std::span<std::int16_t> output,
    std::size_t output_frame_offset,
    std::size_t channel_stride) noexcept {
    if (requested_end < requested_begin || channel_stride == 0) return false;
    const auto begin = std::min(requested_begin, prefix_frames);
    const auto end = std::min(requested_end, prefix_frames);
    if (end <= begin) return true;
    const auto frames = static_cast<std::size_t>(end - begin);
    if (source_end < begin || source_end - begin < frames ||
        source_end > spectrum.size() || output.empty() ||
        output_frame_offset >= output.size() ||
        frames - 1 > (output.size() - 1 - output_frame_offset) / channel_stride) {
        return false;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const auto source_index = source_end - begin - frame - 1;
        const auto output_index = output_frame_offset + frame * channel_stride;
        if (source_index >= spectrum.size() || output_index >= output.size()) return false;
        output[output_index] = vorbis_output_q9_to_s16(spectrum[source_index]);
    }
    return true;
}

// Ports FUN_0100ada2's first mixed region: spectrum A walks backward by two
// values, spectrum B walks backward by one, and the two windows move in
// opposite directions. Cursor indices are resolved by the caller's block plan.
[[nodiscard]] inline bool mix_vorbis_overlap_reverse_window_pair(
    std::span<const std::int32_t> spectrum_a,
    std::size_t spectrum_a_start,
    std::span<const std::int32_t> spectrum_b,
    std::size_t spectrum_b_start,
    std::span<const std::uint8_t> window_a,
    std::size_t window_a_start,
    std::span<const std::uint8_t> window_b,
    std::size_t window_b_end,
    std::size_t frames,
    std::span<std::int16_t> output,
    std::size_t output_frame_offset,
    std::size_t channel_stride) noexcept {
    if (channel_stride == 0) return false;
    if (frames == 0) return true;
    if (frames - 1 > std::numeric_limits<std::size_t>::max() / 2) return false;
    const auto a_backstep = (frames - 1) * 2;
    const auto b_backstep = frames - 1;
    if (spectrum_a_start < a_backstep || spectrum_a_start >= spectrum_a.size() ||
        spectrum_b_start < b_backstep || spectrum_b_start >= spectrum_b.size() ||
        window_a_start > window_a.size() || frames > window_a.size() - window_a_start ||
        window_b_end > window_b.size() || window_b_end < frames || output.empty() ||
        output_frame_offset >= output.size() ||
        frames - 1 > (output.size() - 1 - output_frame_offset) / channel_stride) {
        return false;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const auto left = spectrum_a[spectrum_a_start - frame * 2];
        const auto right = spectrum_b[spectrum_b_start - frame];
        const auto output_index = output_frame_offset + frame * channel_stride;
        output[output_index] = mix_vorbis_windowed_s16(
            left, window_a[window_a_start + frame], right,
            window_b[window_b_end - frame - 1]);
    }
    return true;
}

// Ports the following overlap region: spectrum A advances one value under a
// descending window; spectrum B advances two values under an ascending
// window, and the B term is subtracted from A.
[[nodiscard]] inline bool mix_vorbis_overlap_forward_subtract_region(
    std::span<const std::int32_t> spectrum_a,
    std::size_t spectrum_a_start,
    std::span<const std::uint8_t> window_a,
    std::size_t window_a_end,
    std::span<const std::int32_t> spectrum_b,
    std::size_t spectrum_b_start,
    std::span<const std::uint8_t> window_b,
    std::size_t window_b_start,
    std::size_t frames,
    std::span<std::int16_t> output,
    std::size_t output_frame_offset,
    std::size_t channel_stride) noexcept {
    if (channel_stride == 0) return false;
    if (frames == 0) return true;
    if (frames - 1 > std::numeric_limits<std::size_t>::max() / 2) return false;
    const auto b_backstep = (frames - 1) * 2;
    if (spectrum_a_start >= spectrum_a.size() ||
        frames > spectrum_a.size() - spectrum_a_start ||
        spectrum_b_start >= spectrum_b.size() ||
        b_backstep >= spectrum_b.size() - spectrum_b_start ||
        window_a_end > window_a.size() || window_a_end < frames ||
        window_b_start > window_b.size() || frames > window_b.size() - window_b_start ||
        output.empty() || output_frame_offset >= output.size() ||
        frames - 1 > (output.size() - 1 - output_frame_offset) / channel_stride) {
        return false;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        output[output_frame_offset + frame * channel_stride] = mix_vorbis_windowed_s16(
            spectrum_a[spectrum_a_start + frame],
            window_a[window_a_end - frame - 1],
            spectrum_b[spectrum_b_start + frame * 2],
            window_b[window_b_start + frame], true);
    }
    return true;
}

// Ports the renderer's terminal overlap tail: every other transform value is
// negated, shifted from Q9, saturated, and written at the channel stride.
[[nodiscard]] inline bool copy_vorbis_overlap_negated_tail(
    std::span<const std::int32_t> spectrum,
    std::size_t source_start,
    std::size_t frames,
    std::span<std::int16_t> output,
    std::size_t output_frame_offset,
    std::size_t channel_stride) noexcept {
    if (channel_stride == 0) return false;
    if (frames == 0) return true;
    if (frames - 1 > std::numeric_limits<std::size_t>::max() / 2) return false;
    const auto source_advance = (frames - 1) * 2;
    if (source_start >= spectrum.size() ||
        source_advance >= spectrum.size() - source_start || output.empty() ||
        output_frame_offset >= output.size() ||
        frames - 1 > (output.size() - 1 - output_frame_offset) / channel_stride) {
        return false;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const auto negated = vorbis_output_wrap32(
            -static_cast<std::int64_t>(spectrum[source_start + frame * 2]));
        output[output_frame_offset + frame * channel_stride] =
            vorbis_output_q9_to_s16(negated);
    }
    return true;
}

// Composes FUN_0100ada2's transition prefix, two windowed regions, and tail for
// one channel. Cursors are derived from the recovered block/window branches.
[[nodiscard]] inline std::optional<std::size_t> render_vorbis_overlap_channel(
    std::uint32_t first_block_size,
    std::uint32_t second_block_size,
    bool first_window_long,
    bool second_window_long,
    std::span<const std::int32_t> first_spectrum,
    std::span<const std::int32_t> second_spectrum,
    std::span<const std::uint8_t> first_window,
    std::span<const std::uint8_t> second_window,
    std::uint32_t requested_begin,
    std::uint32_t requested_end,
    std::span<std::int16_t> output,
    std::size_t output_frame_offset,
    std::size_t channel_stride) noexcept {
    const auto plan = plan_vorbis_overlap_regions(
        first_block_size, second_block_size, first_window_long, second_window_long);
    if (!plan || requested_end < requested_begin || channel_stride == 0) {
        return std::nullopt;
    }

    const bool both_long = first_window_long && second_window_long;
    const auto& window = both_long ? second_window : first_window;
    const auto window_size = static_cast<std::int64_t>(
        both_long ? second_block_size / 2 : first_block_size / 2);
    if (window.size() < static_cast<std::size_t>(window_size)) return std::nullopt;

    const auto to_size = [](std::int64_t value) noexcept
        -> std::optional<std::size_t> {
        if (value < 0 || static_cast<std::uint64_t>(value) >
                             std::numeric_limits<std::size_t>::max()) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(value);
    };
    const auto min64 = [](std::int64_t left, std::int64_t right) noexcept {
        return left < right ? left : right;
    };
    const auto output_offset = [&](std::size_t added) noexcept
        -> std::optional<std::size_t> {
        if (added > std::numeric_limits<std::size_t>::max() - output_frame_offset) {
            return std::nullopt;
        }
        return output_frame_offset + added;
    };

    std::int64_t begin = requested_begin;
    std::int64_t end = requested_end;
    std::int64_t second_cursor = plan->spectrum_quarter_offset;
    std::size_t written = 0;
    const auto prefix = static_cast<std::int64_t>(plan->unwindowed_prefix_frames);
    const auto prefix_begin = min64(begin, prefix);
    const auto prefix_end = min64(end, prefix);
    if (prefix > 0 && prefix_end > prefix_begin) {
        if (!copy_vorbis_overlap_prefix(
                second_spectrum, static_cast<std::size_t>(second_cursor),
                static_cast<std::uint32_t>(prefix_begin),
                static_cast<std::uint32_t>(prefix_end),
                static_cast<std::uint32_t>(prefix), output,
                output_frame_offset, channel_stride)) {
            return std::nullopt;
        }
        written = static_cast<std::size_t>(prefix_end - prefix_begin);
    }
    second_cursor -= prefix_end;
    begin -= prefix_begin;
    end -= prefix_end;

    const auto quarter_limit = static_cast<std::int64_t>(plan->selected_half_block / 2);
    std::int64_t first_cursor = plan->selected_half_block;
    std::int64_t window_begin = 0;
    std::int64_t window_end = window_size;

    const auto region1_begin = min64(begin, quarter_limit);
    const auto region1_end = min64(end, quarter_limit);
    const auto region1_frames = region1_end - region1_begin;
    if (region1_frames > 0) {
        const auto a_start = to_size(first_cursor - 2 * region1_begin - 2);
        const auto b_start = to_size(second_cursor - region1_begin - 1);
        const auto wa_start = to_size(window_begin + region1_begin);
        const auto wb_end = to_size(window_end - region1_begin);
        const auto frames = to_size(region1_frames);
        const auto destination = output_offset(written);
        if (!a_start || !b_start || !wa_start || !wb_end || !frames || !destination ||
            !mix_vorbis_overlap_reverse_window_pair(
                first_spectrum, *a_start, second_spectrum, *b_start,
                window, *wa_start, window, *wb_end, *frames, output,
                *destination, channel_stride)) {
            return std::nullopt;
        }
        written += *frames;
    }

    second_cursor -= region1_end;
    first_cursor -= 2 * region1_end;
    window_end -= region1_end;
    window_begin += region1_end;
    begin -= region1_begin;
    end -= region1_end;

    const auto region2_begin = min64(begin, quarter_limit);
    const auto region2_end = min64(end, quarter_limit);
    const auto region2_frames = region2_end - region2_begin;
    if (region2_frames > 0) {
        const auto b_start = to_size(second_cursor + region2_begin);
        const auto a_start = to_size(first_cursor + 2 * region2_begin);
        const auto wa_end = to_size(window_end - region2_begin);
        const auto wb_start = to_size(window_begin + region2_begin);
        const auto frames = to_size(region2_frames);
        const auto destination = output_offset(written);
        if (!a_start || !b_start || !wa_end || !wb_start || !frames || !destination ||
            !mix_vorbis_overlap_forward_subtract_region(
                second_spectrum, *b_start, window, *wa_end,
                first_spectrum, *a_start, window, *wb_start, *frames, output,
                *destination, channel_stride)) {
            return std::nullopt;
        }
        written += *frames;
    }

    first_cursor += 2 * region2_end;
    begin -= region2_begin;
    end -= region2_end;
    const auto alternate = static_cast<std::int64_t>(plan->alternate_window_frames);
    if (alternate > 0) {
        const auto tail_end = min64(end, alternate);
        const auto tail_begin = min64(begin, alternate);
        const auto tail_start = to_size(first_cursor + 2 * tail_begin);
        const auto signed_span = tail_end * 8 - tail_begin * 2 + 4;
        const auto shifted_span = signed_span >> 2;
        std::int64_t tail_frames = 0;
        if (shifted_span > 1) {
            tail_frames = (shifted_span - (signed_span >> 31)) >> 1;
        }
        const auto frames = to_size(tail_frames);
        const auto destination = output_offset(written);
        if (!tail_start || !frames || !destination ||
            !copy_vorbis_overlap_negated_tail(
                first_spectrum, *tail_start, *frames, output,
                *destination, channel_stride)) {
            return std::nullopt;
        }
        written += *frames;
    }
    return written;
}

[[nodiscard]] inline std::optional<std::size_t> render_vorbis_overlap_channel(
    std::uint32_t first_block_size,
    std::uint32_t second_block_size,
    bool first_window_long,
    bool second_window_long,
    const VorbisOverlapChannelBuffers& buffers,
    std::span<const std::uint8_t> first_window,
    std::span<const std::uint8_t> second_window,
    std::uint32_t requested_begin,
    std::uint32_t requested_end,
    std::span<std::int16_t> output,
    std::size_t output_frame_offset,
    std::size_t channel_stride) noexcept {
    if (buffers.first_spectrum.size() < kVorbisFirstSpectrumCapacity ||
        buffers.second_spectrum.size() < kVorbisSecondSpectrumCapacity) {
        return std::nullopt;
    }
    return render_vorbis_overlap_channel(
        first_block_size, second_block_size, first_window_long,
        second_window_long, buffers.first_spectrum, buffers.second_spectrum,
        first_window, second_window, requested_begin, requested_end, output,
        output_frame_offset, channel_stride);
}

// Models FUN_01004cb6's readable sample interval and the byte-size contract
// used by FUN_0100dfcc. The decoder stores 16-bit PCM, interleaved by channel.
[[nodiscard]] inline std::optional<VorbisOutputReadPlan> plan_vorbis_output_read(
    std::int32_t decoded_begin,
    std::int32_t decoded_end,
    std::uint32_t channel_count,
    bool render_pcm,
    std::uint32_t output_capacity_bytes) noexcept {
    if (decoded_begin < 0 || decoded_end <= decoded_begin || channel_count == 0 ||
        channel_count > std::numeric_limits<std::uint32_t>::max() / 2U) {
        return std::nullopt;
    }

    std::uint32_t frames = static_cast<std::uint32_t>(decoded_end - decoded_begin);
    if (render_pcm) {
        const std::uint32_t capacity_frames =
            output_capacity_bytes / (2U * channel_count);
        if (frames > capacity_frames) frames = capacity_frames;
    }

    const std::uint64_t bytes =
        static_cast<std::uint64_t>(frames) * channel_count * sizeof(std::int16_t);
    if (bytes > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    return VorbisOutputReadPlan{frames, static_cast<std::uint32_t>(bytes), render_pcm};
}

// Models the successful branch of FUN_0100dfcc after FUN_01004cb6 has
// produced a positive frame count. The decoder advances its sample cursor,
// adds frames to the low/high 64-bit consumed-sample counter, optionally
// reports the current output item, and returns interleaved s16 byte length.
[[nodiscard]] inline std::optional<VorbisOutputReadCommit> commit_vorbis_output_read(
    const VorbisOutputReadPlan& read,
    std::uint64_t consumed_samples,
    std::optional<std::uint32_t> output_item_index = std::nullopt) noexcept {
    if (read.sample_frames == 0) return std::nullopt;
    const auto next_consumed = consumed_samples + read.sample_frames;
    return VorbisOutputReadCommit{
        read.sample_frames, read.pcm_bytes, next_consumed, output_item_index};
}

} // namespace echo::audio
