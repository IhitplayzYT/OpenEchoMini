#include "echo/audio/lrc_sidecar.hpp"

#include <cstddef>

namespace echo::audio {
namespace {

constexpr unsigned char ascii_upper(unsigned char value) noexcept {
    return value >= 'a' && value <= 'z'
               ? static_cast<unsigned char>(value - ('a' - 'A'))
               : value;
}

} // namespace

bool matches_firmware_lrc_candidate(std::string_view audio_name,
                                    std::u16string_view candidate_name,
                                    bool sidecar_scan_enabled) noexcept {
    if (!sidecar_scan_enabled || audio_name.size() < 5 ||
        candidate_name.size() < 3) {
        return false;
    }

    constexpr std::string_view suffix = "LRC";
    const auto prefix_size = audio_name.size() - 4;
    if (candidate_name.size() < prefix_size) return false;

    const auto candidate_suffix = candidate_name.substr(
        candidate_name.size() - suffix.size());
    for (std::size_t i = 0; i < suffix.size(); ++i) {
        if (ascii_upper(static_cast<unsigned char>(candidate_suffix[i] & 0xffu)) !=
            static_cast<unsigned char>(suffix[i])) {
            return false;
        }
    }

    for (std::size_t i = 0; i < prefix_size; ++i) {
        if (ascii_upper(static_cast<unsigned char>(audio_name[i])) !=
            ascii_upper(static_cast<unsigned char>(candidate_name[i] & 0xffu))) {
            return false;
        }
    }
    return true;
}

} // namespace echo::audio
