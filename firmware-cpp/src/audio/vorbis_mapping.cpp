#include "echo/audio/vorbis_mapping.hpp"

#include <cstdint>

namespace echo::audio {
namespace {

class LsbBitReader {
public:
    explicit LsbBitReader(std::span<const std::uint8_t> bytes) noexcept
        : bytes_(bytes) {}

    [[nodiscard]] bool read(std::uint8_t count, std::uint32_t& value) noexcept {
        if (count > 32) return false;
        value = 0;
        for (std::uint8_t i = 0; i < count; ++i) {
            const auto byte_index = bit_offset_ / 8;
            if (byte_index >= bytes_.size()) return false;
            const auto bit_index = static_cast<unsigned>(bit_offset_ % 8);
            const auto bit = (bytes_[byte_index] >> bit_index) & 1U;
            value |= static_cast<std::uint32_t>(bit) << i;
            ++bit_offset_;
        }
        return true;
    }

    [[nodiscard]] std::size_t bit_offset() const noexcept { return bit_offset_; }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t bit_offset_{};
};

std::uint8_t channel_index_width(std::uint8_t channel_count) noexcept {
    std::uint32_t value = channel_count == 0 ? 0 : channel_count - 1;
    std::uint8_t width = 0;
    while (value != 0) {
        ++width;
        value >>= 1U;
    }
    return width;
}

} // namespace

std::optional<VorbisMapping0> parse_vorbis_mapping0(
    std::span<const std::uint8_t> mapping_bits,
    std::uint8_t channel_count,
    std::uint16_t floor_count,
    std::uint16_t residue_count) {
    if (channel_count == 0) return std::nullopt;

    LsbBitReader bits(mapping_bits);
    std::uint32_t value = 0;
    if (!bits.read(1, value)) return std::nullopt;
    const auto submap_count = value == 0 ? 1U : [&]() -> std::uint32_t {
        std::uint32_t count = 0;
        if (!bits.read(4, count)) return 0;
        return count + 1;
    }();
    if (submap_count == 0) return std::nullopt;

    VorbisMapping0 mapping;
    std::uint32_t coupling_flag = 0;
    if (!bits.read(1, coupling_flag)) return std::nullopt;
    if (coupling_flag != 0) {
        std::uint32_t coupling_count_minus_one = 0;
        if (!bits.read(8, coupling_count_minus_one)) return std::nullopt;
        const auto coupling_count = coupling_count_minus_one + 1;
        const auto width = channel_index_width(channel_count);
        mapping.coupling_steps.reserve(coupling_count);
        for (std::uint32_t i = 0; i < coupling_count; ++i) {
            std::uint32_t magnitude = 0;
            std::uint32_t angle = 0;
            if (!bits.read(width, magnitude) || !bits.read(width, angle) ||
                magnitude >= channel_count || angle >= channel_count ||
                magnitude == angle) {
                return std::nullopt;
            }
            mapping.coupling_steps.emplace_back(
                static_cast<std::uint8_t>(magnitude), static_cast<std::uint8_t>(angle));
        }
    }

    std::uint32_t reserved = 0;
    if (!bits.read(2, reserved) || reserved != 0) return std::nullopt;

    mapping.channel_mux.resize(channel_count, 0);
    if (submap_count > 1) {
        for (auto& mux : mapping.channel_mux) {
            if (!bits.read(4, value) || value >= submap_count) return std::nullopt;
            mux = static_cast<std::uint8_t>(value);
        }
    }

    mapping.submaps.reserve(submap_count);
    for (std::uint32_t i = 0; i < submap_count; ++i) {
        std::uint32_t time_configuration = 0;
        std::uint32_t floor_index = 0;
        std::uint32_t residue_index = 0;
        if (!bits.read(8, time_configuration) || !bits.read(8, floor_index) ||
            !bits.read(8, residue_index) || floor_index >= floor_count ||
            residue_index >= residue_count) {
            return std::nullopt;
        }
        mapping.submaps.push_back({static_cast<std::uint8_t>(time_configuration),
                                   static_cast<std::uint8_t>(floor_index),
                                   static_cast<std::uint8_t>(residue_index)});
    }
    mapping.bits_consumed = bits.bit_offset();
    return mapping;
}

} // namespace echo::audio
