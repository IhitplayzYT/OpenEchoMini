#include "echo/audio/wav_container.hpp"

#include <array>
#include <limits>
#include <stdexcept>

namespace echo::audio {
namespace {

bool has_id(std::span<const std::uint8_t> bytes, std::size_t offset,
            char a, char b, char c, char d) noexcept {
    return offset <= bytes.size() && bytes.size() - offset >= 4
        && bytes[offset] == static_cast<std::uint8_t>(a)
        && bytes[offset + 1] == static_cast<std::uint8_t>(b)
        && bytes[offset + 2] == static_cast<std::uint8_t>(c)
        && bytes[offset + 3] == static_cast<std::uint8_t>(d);
}

std::uint16_t read_le16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2) {
        throw std::runtime_error("truncated WAVE 16-bit field");
    }
    return static_cast<std::uint16_t>(bytes[offset]
        | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8));
}

std::uint32_t read_le32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("truncated WAVE 32-bit field");
    }
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

} // namespace

WavInfo WavContainerParser::parse(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 12 || !has_id(bytes, 0, 'R', 'I', 'F', 'F')
        || !has_id(bytes, 8, 'W', 'A', 'V', 'E')) {
        throw std::runtime_error("not a RIFF/WAVE container");
    }

    const auto riff_size = read_le32(bytes, 4);
    if (riff_size < 4 || riff_size > std::numeric_limits<std::size_t>::max() - 8) {
        throw std::runtime_error("invalid RIFF size");
    }
    const auto end = static_cast<std::size_t>(riff_size) + 8;
    if (end > bytes.size()) throw std::runtime_error("truncated RIFF container");

    WavInfo info;
    info.riff_bytes = riff_size;
    bool have_format = false;
    bool have_data = false;
    std::size_t cursor = 12;
    while (cursor < end) {
        if (end - cursor < 8) throw std::runtime_error("truncated RIFF chunk header");
        const auto payload_size = read_le32(bytes, cursor + 4);
        const auto payload = cursor + 8;
        if (payload_size > end - payload) throw std::runtime_error("RIFF chunk exceeds container");
        const auto payload_end = payload + payload_size;
        const auto padded_end = payload_end + (payload_size & 1u);
        if (padded_end > end) throw std::runtime_error("truncated RIFF chunk padding");

        if (has_id(bytes, cursor, 'f', 'm', 't', ' ')) {
            if (payload_size < 16) throw std::runtime_error("short WAVE fmt chunk");
            info.format_tag = read_le16(bytes, payload);
            info.channels = read_le16(bytes, payload + 2);
            info.sample_rate = read_le32(bytes, payload + 4);
            info.average_bytes_per_second = read_le32(bytes, payload + 8);
            info.block_align = read_le16(bytes, payload + 12);
            info.bits_per_sample = read_le16(bytes, payload + 14);
            have_format = true;
        } else if (has_id(bytes, cursor, 'd', 'a', 't', 'a')) {
            info.data_offset = payload;
            info.data_bytes = payload_size;
            have_data = true;
        }
        cursor = padded_end;
    }

    if (!have_format || !have_data) throw std::runtime_error("WAVE fmt or data chunk missing");
    if (info.block_align != 0) info.complete_frames = info.data_bytes / info.block_align;
    return info;
}

bool is_firmware_wav_sample_rate(std::uint32_t sample_rate) noexcept {
    constexpr std::array<std::uint32_t, 16> rates{
        6000, 8000, 11025, 12000, 16000, 22050, 24000, 32000,
        44100, 48000, 64000, 88200, 96000, 128000, 176400, 192000,
    };
    for (const auto rate : rates) {
        if (rate == sample_rate) return true;
    }
    return false;
}

bool is_firmware_wav_format_supported(
    std::span<const std::uint8_t> fmt_payload) noexcept {
    const auto le16 = [fmt_payload](std::size_t offset) noexcept {
        return static_cast<std::uint16_t>(fmt_payload[offset]) |
               (static_cast<std::uint16_t>(fmt_payload[offset + 1]) << 8U);
    };
    const auto le32 = [fmt_payload](std::size_t offset) noexcept {
        return static_cast<std::uint32_t>(fmt_payload[offset]) |
               (static_cast<std::uint32_t>(fmt_payload[offset + 1]) << 8U) |
               (static_cast<std::uint32_t>(fmt_payload[offset + 2]) << 16U) |
               (static_cast<std::uint32_t>(fmt_payload[offset + 3]) << 24U);
    };
    const auto bits_allowed = [](std::uint16_t bits) noexcept {
        return bits == 8 || bits == 16 || bits == 24 || bits == 32;
    };

    if (fmt_payload.size() < 16) return false;
    const auto format_tag = le16(0);
    const auto channels = le16(2);
    const auto block_align = le16(12);
    const auto bits_per_sample = le16(14);

    // The firmware's branch accepts one or two channels and applies the
    // alignment limit inside that same channel-count branch.
    if (channels > 2) return false;

    if (format_tag == 0x0001) {
        return block_align <= 0x3000 && bits_allowed(bits_per_sample);
    }

    if (format_tag == 0x0002) { // Microsoft ADPCM
        if (fmt_payload.size() != 0x32 || block_align > 0x1000 ||
            bits_per_sample != 4 || le16(16) != 0x20) {
            return false;
        }
        const auto samples_per_block = le16(18);
        const auto coefficient_count = le16(20);
        return samples_per_block == 0x800 ||
               (samples_per_block < 0x801 && coefficient_count == 7);
    }

    if (format_tag == 0x0011) { // IMA/DVI ADPCM
        if (fmt_payload.size() < 20 || block_align > 0x1000 ||
            le16(16) != 2 || le16(18) > 0x1000) {
            return false;
        }
        return bits_per_sample == 3 || bits_per_sample == 4;
    }

    if (format_tag == 0xfffe) { // WAVE_FORMAT_EXTENSIBLE, PCM subformat
        constexpr std::array<std::uint8_t, 12> pcm_guid_tail{
            0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
            0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,
        };
        if (fmt_payload.size() < 40 || block_align > 0x1000 ||
            le16(24) != 1 || !bits_allowed(bits_per_sample)) {
            return false;
        }
        for (std::size_t i = 0; i < pcm_guid_tail.size(); ++i) {
            if (fmt_payload[28 + i] != pcm_guid_tail[i]) return false;
        }
        return true;
    }

    return false;
}

} // namespace echo::audio
