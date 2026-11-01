#include "echo/audio/format_label.hpp"

#include <string>

namespace echo::audio {

std::string_view playback_format_label(std::uint16_t codec_id) noexcept {
    switch (codec_id) {
    case 0: return "MP3";
    case 1: return "WMA";
    case 2: return "AAC";
    case 3: return "WAV";
    case 4: return "OGG";
    case 5: return "APE";
    case 6: return "FALC";
    case 7: return "AALC";
    case 8: return "DSD";
    case 9: return "DSF";
    default: return {};
    }
}

namespace {

void append_fixed_decimal(std::string& output, std::uint32_t value,
                          unsigned width) {
    std::uint32_t divisor = 1;
    for (unsigned i = 1; i < width; ++i) divisor *= 10u;
    value %= divisor * 10u;
    for (; divisor != 0; divisor /= 10u) {
        output.push_back(static_cast<char>('0' + (value / divisor) % 10u));
    }
}

std::uint32_t dsd_rate_tenths_mhz(std::uint32_t bitrate) noexcept {
    // The binary compares unsigned 32-bit additions against 1000. The three
    // constants are -5,645,000, -11,290,000, and -22,579,000.
    if (bitrate + std::uint32_t{0xffa9dd38u} < 1000u) return 28;
    if (bitrate + std::uint32_t{0xff53ba70u} >= 1000u) {
        return bitrate + std::uint32_t{0xfea778c8u} >= 1000u ? 28u : 112u;
    }
    return 56;
}

} // namespace

PlaybackFormatDisplay format_playback_information(
    std::uint16_t codec_id, std::uint32_t bitrate_bits_per_second,
    std::uint32_t sample_rate_hz, std::uint32_t bit_depth) {
    PlaybackFormatDisplay display{};
    display.dsd = bit_depth < 10u;
    display.text.append(playback_format_label(codec_id));
    display.text.push_back('|');

    if (display.dsd) {
        const auto dsd_rate = dsd_rate_tenths_mhz(bitrate_bits_per_second);
        if (dsd_rate >= 100u) {
            append_fixed_decimal(display.text, dsd_rate / 10u, 2u);
        } else {
            display.text.push_back(static_cast<char>('0' + dsd_rate / 10u));
        }
        display.text.push_back('.');
        display.text.push_back(static_cast<char>('0' + dsd_rate % 10u));
        display.text.append("MHz|");
        display.text.append(std::to_string(bit_depth));
        display.text.append("bit|DSD");
        display.quality_icon_id = 0x13f;
        return display;
    }

    const auto sample_rate_khz = sample_rate_hz / 1000u;
    append_fixed_decimal(display.text, sample_rate_khz,
                         sample_rate_khz > 99u ? 3u : 2u);
    display.text.append("KHz|");
    append_fixed_decimal(display.text, bit_depth, 2u);
    display.text.append("bit|");

    const auto bitrate_kbps = bitrate_bits_per_second / 1000u;
    const auto bitrate_width = bitrate_bits_per_second > 999u
                                   ? 4u
                                   : (bitrate_bits_per_second > 99u ? 3u : 2u);
    append_fixed_decimal(display.text, bitrate_kbps, bitrate_width);
    display.text.append("kbps");

    const bool high_resolution_codec = codec_id == 3u || codec_id == 5u ||
                                       codec_id == 6u || codec_id == 7u;
    if (!high_resolution_codec) {
        display.quality_icon_id = 0x140;
    } else if (bit_depth < 24u) {
        if (sample_rate_hz >= 96000u) {
            display.quality_icon_id = 0x13e;
        } else if (sample_rate_hz < 44100u) {
            display.quality_icon_id = 0x140;
        } else {
            display.quality_icon_id = 0x13d;
        }
    } else {
        display.quality_icon_id = sample_rate_hz < 44100u ? 0x140u : 0x13eu;
    }
    return display;
}

} // namespace echo::audio
