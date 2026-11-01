#include "echo/audio/mpeg_frame_header.hpp"

namespace echo::audio {

std::optional<MpegFrameHeader> parse_mpeg_frame_header(
    const std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < 4) {
        return std::nullopt;
    }

    const auto header = (static_cast<std::uint32_t>(bytes[0]) << 24U)
        | (static_cast<std::uint32_t>(bytes[1]) << 16U)
        | (static_cast<std::uint32_t>(bytes[2]) << 8U)
        | static_cast<std::uint32_t>(bytes[3]);
    if ((header >> 21U) != 0x7ffU) {
        return std::nullopt;
    }

    const auto version_bits = static_cast<std::uint8_t>((header >> 19U) & 0x3U);
    const auto layer_bits = static_cast<std::uint8_t>((header >> 17U) & 0x3U);
    const auto bitrate_index = static_cast<std::uint8_t>((header >> 12U) & 0xfU);
    const auto sample_rate_index = static_cast<std::uint8_t>((header >> 10U) & 0x3U);
    const auto emphasis = static_cast<std::uint8_t>(header & 0x3U);
    if (version_bits == 1 || layer_bits == 0 || bitrate_index == 0xf
        || sample_rate_index == 3 || emphasis == 2) {
        return std::nullopt;
    }

    const auto version = version_bits == 3 ? MpegVersion::v1
        : version_bits == 2 ? MpegVersion::v2 : MpegVersion::v2_5;
    const auto layer = layer_bits == 3 ? MpegLayer::layer1
        : layer_bits == 2 ? MpegLayer::layer2 : MpegLayer::layer3;
    constexpr std::uint32_t base_rates[] = {44100, 48000, 32000};
    auto sample_rate = base_rates[sample_rate_index];
    if (version == MpegVersion::v2) {
        sample_rate /= 2;
    } else if (version == MpegVersion::v2_5) {
        sample_rate /= 4;
    }

    const auto channel_mode = static_cast<std::uint8_t>((header >> 6U) & 0x3U);
    return MpegFrameHeader{
        version,
        layer,
        bitrate_index,
        sample_rate_index,
        sample_rate,
        channel_mode,
        static_cast<std::uint8_t>((header >> 4U) & 0x3U),
        ((header >> 16U) & 1U) == 0,
        ((header >> 9U) & 1U) != 0,
    };
}

std::optional<std::size_t> find_mpeg_sync(
    const std::span<const std::uint8_t> bytes,
    const std::size_t start_offset) noexcept {
    if (start_offset > bytes.size() || bytes.size() - start_offset < 8) {
        return std::nullopt;
    }
    for (auto offset = start_offset; offset <= bytes.size() - 8; ++offset) {
        if (bytes[offset] == 0xff && (bytes[offset + 1] & 0xe0U) == 0xe0U) {
            return offset;
        }
    }
    return std::nullopt;
}

} // namespace echo::audio
