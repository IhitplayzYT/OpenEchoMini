#pragma once

#include <string_view>

namespace echo::audio {

// Mirrors module 27's filename scan: the audio name must be at least five
// bytes, its final four bytes are ignored, the candidate begins with the
// remaining prefix, and its last three characters are LRC (ASCII case
// insensitive). The source does not verify the separator before LRC.
[[nodiscard]] bool matches_firmware_lrc_candidate(
    std::string_view audio_name, std::u16string_view candidate_name,
    bool sidecar_scan_enabled) noexcept;

} // namespace echo::audio
