#include "echo/audio/flac_frame_header.hpp"

#include <array>
#include <limits>

namespace echo::audio {
namespace {

std::optional<std::uint64_t> read_coded_number(
    const std::span<const std::uint8_t> bytes, std::size_t& position) noexcept {
    if (position >= bytes.size()) return std::nullopt;
    const auto first = bytes[position++];
    if ((first & 0x80U) == 0U) return first;

    unsigned continuation_count = 0;
    std::uint64_t value = 0;
    std::uint8_t payload_mask = 0;
    for (std::uint8_t mask = 0x40U; mask != 0U && (first & mask) != 0U;
         mask = static_cast<std::uint8_t>(mask >> 1U)) {
        ++continuation_count;
    }
    if (continuation_count == 0U || continuation_count > 6U) return std::nullopt;

    payload_mask = static_cast<std::uint8_t>((1U << (6U - continuation_count)) - 1U);
    value = first & payload_mask;
    for (unsigned i = 0; i < continuation_count; ++i) {
        if (position >= bytes.size()) return std::nullopt;
        const auto next = bytes[position++];
        if ((next & 0xc0U) != 0x80U) return std::nullopt;
        if (value > (std::numeric_limits<std::uint64_t>::max() >> 6U)) {
            return std::nullopt;
        }
        value = (value << 6U) | (next & 0x3fU);
    }
    constexpr std::array<std::uint64_t, 7> minimums{
        0U, 0x80U, 0x800U, 0x10000U, 0x200000U, 0x4000000U, 0x80000000U};
    if (value < minimums[continuation_count]) return std::nullopt;
    return value;
}

std::optional<std::uint8_t> resolve_bits_per_sample(
    const std::uint8_t code, const FlacStreamDefaults defaults) noexcept {
    constexpr std::array<std::uint8_t, 8> values{0, 8, 12, 0, 16, 20, 24, 0};
    if (code == 0U) return defaults.bits_per_sample;
    const auto value = values[code];
    return value == 0U ? std::nullopt : std::optional<std::uint8_t>{value};
}

std::uint8_t crc8(const std::span<const std::uint8_t> bytes) noexcept {
    std::uint8_t crc = 0;
    for (const auto byte : bytes) {
        crc = static_cast<std::uint8_t>(crc ^ byte);
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc = static_cast<std::uint8_t>((crc & 0x80U) != 0U
                ? static_cast<std::uint8_t>((crc << 1U) ^ 0x07U)
                : static_cast<std::uint8_t>(crc << 1U));
        }
    }
    return crc;
}

} // namespace

std::optional<FlacFrameHeader> parse_flac_frame_header(
    const std::span<const std::uint8_t> bytes,
    const FlacStreamDefaults defaults,
    const bool verify_crc) noexcept {
    if (bytes.size() < 6U || bytes[0] != 0xffU || bytes[1] != 0xf8U) {
        return std::nullopt;
    }

    const auto block_code = static_cast<std::uint8_t>(bytes[2] >> 4U);
    const auto sample_rate_code = static_cast<std::uint8_t>(bytes[2] & 0x0fU);
    const auto channel_code = static_cast<std::uint8_t>(bytes[3] >> 4U);
    const auto bit_depth_code = static_cast<std::uint8_t>((bytes[3] >> 1U) & 0x07U);
    if ((bytes[3] & 1U) != 0U) return std::nullopt;

    FlacFrameHeader result{};
    if (channel_code == 0U) {
        result.channels = 1;
    } else if (channel_code == 1U) {
        result.channels = 2;
    } else if (channel_code >= 8U && channel_code <= 10U) {
        result.channels = 2;
    } else {
        return std::nullopt;
    }
    result.channel_assignment = static_cast<FlacChannelAssignment>(channel_code);

    const auto bits_per_sample = resolve_bits_per_sample(bit_depth_code, defaults);
    if (!bits_per_sample || *bits_per_sample == 0U) return std::nullopt;
    result.bits_per_sample = *bits_per_sample;

    std::size_t position = 4U;
    const auto frame_number = read_coded_number(bytes, position);
    if (!frame_number || *frame_number > 0x7fffffffU) return std::nullopt;
    result.frame_number = *frame_number;

    switch (block_code) {
    case 0:
        return std::nullopt;
    case 1:
        result.block_size = 192U;
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        result.block_size = 144U << block_code;
        break;
    case 6:
        if (position >= bytes.size()) return std::nullopt;
        result.block_size = static_cast<std::uint32_t>(bytes[position++]) + 1U;
        break;
    case 7:
        if (bytes.size() - position < 2U) return std::nullopt;
        result.block_size = (static_cast<std::uint32_t>(bytes[position]) << 8U)
            | bytes[position + 1U];
        ++result.block_size;
        position += 2U;
        break;
    default:
        result.block_size = 1U << block_code;
        break;
    }

    constexpr std::array<std::uint32_t, 12> sample_rates{
        0U, 88200U, 176400U, 192000U, 8000U, 16000U,
        22050U, 24000U, 32000U, 44100U, 48000U, 96000U};
    if (sample_rate_code < 12U) {
        result.sample_rate = sample_rate_code == 0U
            ? defaults.sample_rate.value_or(0U)
            : sample_rates[sample_rate_code];
    } else if (sample_rate_code == 12U) {
        if (position >= bytes.size()) return std::nullopt;
        result.sample_rate = static_cast<std::uint32_t>(bytes[position++]) * 1000U;
    } else if (sample_rate_code == 13U || sample_rate_code == 14U) {
        if (bytes.size() - position < 2U) return std::nullopt;
        result.sample_rate = (static_cast<std::uint32_t>(bytes[position]) << 8U)
            | bytes[position + 1U];
        position += 2U;
        if (sample_rate_code == 14U) result.sample_rate *= 10U;
    } else {
        return std::nullopt;
    }

    if (result.block_size < 16U || result.block_size > 0x1200U
        || result.sample_rate == 0U || position >= bytes.size()) {
        return std::nullopt;
    }
    result.header_crc = bytes[position];
    if (verify_crc && crc8(bytes.first(position)) != result.header_crc) {
        return std::nullopt;
    }
    result.encoded_size = position + 1U;
    return result;
}

} // namespace echo::audio
