#include "echo/audio/mpeg_reservoir.hpp"

#include <algorithm>
#include <limits>

namespace echo::audio {

std::optional<ReservoirCopyResult> copy_mpeg_reservoir_blocks(
    const std::span<const std::uint8_t> reservoir,
    const std::uint8_t first_slot,
    const std::size_t requested_blocks,
    const std::span<std::uint8_t> output) noexcept {
    constexpr std::size_t slot_count = 16;
    constexpr std::size_t block_size = 0x80;
    constexpr std::size_t ring_size = slot_count * block_size;
    if (first_slot >= slot_count || reservoir.size() < ring_size) {
        return std::nullopt;
    }

    const auto count = std::min(requested_blocks, output.size() / block_size);
    for (std::size_t i = 0; i < count; ++i) {
        const auto slot = (static_cast<std::size_t>(first_slot) + i) % slot_count;
        std::copy_n(reservoir.begin() + static_cast<std::ptrdiff_t>(slot * block_size),
                    block_size,
                    output.begin() + static_cast<std::ptrdiff_t>(i * block_size));
    }

    return ReservoirCopyResult{
        count,
        static_cast<std::uint8_t>((static_cast<std::size_t>(first_slot) + count)
                                  % slot_count),
    };
}

std::optional<std::span<const std::uint8_t>>
resolve_mpeg_layer3_main_data_window(
    const std::span<const std::uint8_t> previous_main_data,
    const std::span<const std::uint8_t> current_main_data,
    const std::uint16_t main_data_begin,
    const std::span<std::uint8_t> output) noexcept {
    const std::size_t back_bytes = main_data_begin;
    if (back_bytes > previous_main_data.size() ||
        current_main_data.size() >
            std::numeric_limits<std::size_t>::max() - back_bytes) {
        return std::nullopt;
    }
    const std::size_t window_size = back_bytes + current_main_data.size();
    if (window_size > output.size()) return std::nullopt;
    std::copy(previous_main_data.end() - static_cast<std::ptrdiff_t>(back_bytes),
              previous_main_data.end(), output.begin());
    std::copy(current_main_data.begin(), current_main_data.end(),
              output.begin() + static_cast<std::ptrdiff_t>(back_bytes));
    return std::span<const std::uint8_t>(output.data(), window_size);
}

std::optional<std::span<const std::uint8_t>>
update_mpeg_layer3_reservoir_tail(
    const std::span<const std::uint8_t> previous_main_data,
    const std::span<const std::uint8_t> current_main_data,
    const std::span<std::uint8_t> output) noexcept {
    if (current_main_data.size() >
        std::numeric_limits<std::size_t>::max() - previous_main_data.size()) {
        return std::nullopt;
    }
    const std::size_t total_size = previous_main_data.size() + current_main_data.size();
    const std::size_t retained_size = std::min(total_size, output.size());
    const std::size_t skip = total_size - retained_size;
    std::size_t written = 0;
    if (skip < previous_main_data.size()) {
        const std::size_t previous_offset = skip;
        const std::size_t previous_count = previous_main_data.size() - previous_offset;
        std::copy(previous_main_data.begin() +
                      static_cast<std::ptrdiff_t>(previous_offset),
                  previous_main_data.end(), output.begin());
        written += previous_count;
        std::copy(current_main_data.begin(), current_main_data.end(),
                  output.begin() + static_cast<std::ptrdiff_t>(written));
    } else {
        const std::size_t current_offset = skip - previous_main_data.size();
        std::copy(current_main_data.begin() +
                      static_cast<std::ptrdiff_t>(current_offset),
                  current_main_data.end(), output.begin());
    }
    return std::span<const std::uint8_t>(output.data(), retained_size);
}

} // namespace echo::audio
