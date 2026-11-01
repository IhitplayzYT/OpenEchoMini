#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

enum class PlaybackTimeRecordKind {
    minutes_seconds,
    hours_minutes_seconds,
};

struct PlaybackTimeRecord {
    std::uint32_t seconds{};
    std::size_t consumed{};
    PlaybackTimeRecordKind kind{PlaybackTimeRecordKind::minutes_seconds};
};

// Parses the fixed-position record consumed by module 27's FUN_0307bb2a.
// Index zero is the record's leading metadata byte; time characters begin at
// index one. The hours form validates only the final two second digits, as the
// firmware does. Returns false when the bounded input is too short or those
// two digits are invalid; the firmware's fallback parser is caller-owned.
[[nodiscard]] bool parse_playback_time_record(
    std::span<const std::uint8_t> record,
    PlaybackTimeRecord& result) noexcept;

struct PlaybackTimeWindow {
    std::optional<std::uint32_t> previous_seconds;
    std::optional<std::uint32_t> next_seconds;
};

// Selects the closest parsed timestamps around current playback time. This
// represents the bounds tracked by the LRC scan; file I/O and line decoding
// are outside this helper.
[[nodiscard]] PlaybackTimeWindow find_playback_time_window(
    std::span<const std::uint32_t> timestamps_seconds,
    std::uint32_t current_milliseconds) noexcept;

} // namespace echo::audio
