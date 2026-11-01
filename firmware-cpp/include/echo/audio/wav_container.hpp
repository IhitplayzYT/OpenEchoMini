#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct WavInfo {
    std::uint32_t riff_bytes{};
    std::uint16_t format_tag{};
    std::uint16_t channels{};
    std::uint32_t sample_rate{};
    std::uint32_t average_bytes_per_second{};
    std::uint16_t block_align{};
    std::uint16_t bits_per_sample{};
    std::size_t data_offset{};
    std::uint32_t data_bytes{};
    std::optional<std::uint64_t> complete_frames;
};

// Parses RIFF/WAVE and reports fmt/data chunk metadata. It does not decode PCM
// or the compressed ADPCM formats also recognized by the firmware.
class WavContainerParser {
public:
    [[nodiscard]] static WavInfo parse(std::span<const std::uint8_t> bytes);
};

// Observed module 76 sample-rate whitelist for its WAV initialization path.
[[nodiscard]] bool is_firmware_wav_sample_rate(std::uint32_t sample_rate) noexcept;

// Checks only the format-header predicates observed in module 76. This does
// not mean the corresponding PCM/ADPCM decoder path is fully reconstructed.
[[nodiscard]] bool is_firmware_wav_format_supported(
    std::span<const std::uint8_t> fmt_payload) noexcept;

} // namespace echo::audio
