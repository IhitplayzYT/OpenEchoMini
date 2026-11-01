#include "echo/audio/playback_time.hpp"

namespace echo::audio {
namespace {

constexpr std::uint32_t ascii_zero = 0x30u;
constexpr std::uint32_t minutes_seconds_ascii_correction = 528u;
constexpr std::uint32_t hours_record_ascii_correction = 0x0d50u;

} // namespace

bool parse_playback_time_record(std::span<const std::uint8_t> record,
                                PlaybackTimeRecord& result) noexcept {
    if (record.size() < 7) return false;

    PlaybackTimeRecord parsed{};
    if (record[6] != ':') {
        const auto minutes =
            (static_cast<std::uint32_t>(record[1]) - ascii_zero) * 10u -
            ascii_zero + static_cast<std::uint32_t>(record[2]);
        const auto seconds =
            static_cast<std::uint32_t>(record[4]) * 10u + record[5] -
            minutes_seconds_ascii_correction;
        parsed.seconds = minutes * 60u + seconds;
        parsed.consumed = 5;
        parsed.kind = PlaybackTimeRecordKind::minutes_seconds;
        result = parsed;
        return true;
    }

    if (record.size() < 9) return false;
    if (record[7] < '0' || record[7] > '9' ||
        record[8] < '0' || record[8] > '9') {
        return false;
    }

    const auto hours =
        (static_cast<std::uint32_t>(record[1]) - ascii_zero) * 10u -
        ascii_zero + static_cast<std::uint32_t>(record[2]);
    const auto minutes =
        (static_cast<std::uint32_t>(record[4]) - ascii_zero) * 10u +
        static_cast<std::uint32_t>(record[5]);
    const auto seconds = static_cast<std::uint32_t>(record[7]) * 10u +
                         static_cast<std::uint32_t>(record[8]) -
                         hours_record_ascii_correction;
    parsed.seconds = hours * 3600u + minutes * 60u + seconds;
    parsed.consumed = 8;
    parsed.kind = PlaybackTimeRecordKind::hours_minutes_seconds;
    result = parsed;
    return true;
}

PlaybackTimeWindow find_playback_time_window(
    std::span<const std::uint32_t> timestamps_seconds,
    std::uint32_t current_milliseconds) noexcept {
    PlaybackTimeWindow window{};
    const auto current_seconds = current_milliseconds / 1000u;
    for (const auto timestamp : timestamps_seconds) {
        if (timestamp == 0xffffffffu) continue;
        if (timestamp <= current_seconds) {
            if (!window.previous_seconds ||
                timestamp > *window.previous_seconds) {
                window.previous_seconds = timestamp;
            }
        } else if (!window.next_seconds || timestamp < *window.next_seconds) {
            window.next_seconds = timestamp;
        }
    }
    return window;
}

} // namespace echo::audio
