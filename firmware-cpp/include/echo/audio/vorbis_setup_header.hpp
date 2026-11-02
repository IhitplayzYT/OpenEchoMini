#pragma once

#include "echo/audio/vorbis_audio_packet.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace echo::audio {

struct VorbisSetupHeader {
    VorbisAudioSetupContext decoder;
    std::uint8_t time_configuration_count{};
    std::size_t bits_consumed{};
};

namespace detail {

class VorbisSetupBitCursor {
public:
    explicit VorbisSetupBitCursor(std::span<const std::uint8_t> bytes,
                                  std::size_t offset = 0) noexcept
        : bytes_(bytes), offset_(offset) {}

    [[nodiscard]] bool read(std::uint8_t width, std::uint32_t& value) noexcept {
        if (width > 32 || offset_ > bytes_.size() * 8U ||
            width > bytes_.size() * 8U - offset_) {
            return false;
        }
        value = 0;
        for (std::uint8_t bit = 0; bit < width; ++bit, ++offset_) {
            const auto byte_index = offset_ / 8U;
            const auto bit_index = static_cast<unsigned>(offset_ % 8U);
            value |= static_cast<std::uint32_t>((bytes_[byte_index] >> bit_index) & 1U) << bit;
        }
        return true;
    }

    [[nodiscard]] bool skip(std::size_t bits) noexcept {
        if (offset_ > bytes_.size() * 8U || bits > bytes_.size() * 8U - offset_)
            return false;
        offset_ += bits;
        return true;
    }

    [[nodiscard]] std::vector<std::uint8_t> remaining_rebased() const {
        if (offset_ > bytes_.size() * 8U) return {};
        const auto remaining = bytes_.size() * 8U - offset_;
        std::vector<std::uint8_t> result((remaining + 7U) / 8U, 0);
        for (std::size_t bit = 0; bit < remaining; ++bit) {
            const auto source = offset_ + bit;
            const auto value = (bytes_[source / 8U] >> (source % 8U)) & 1U;
            result[bit / 8U] |= static_cast<std::uint8_t>(value << (bit % 8U));
        }
        return result;
    }

    [[nodiscard]] std::size_t offset() const noexcept { return offset_; }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_{};
};

} // namespace detail

// Parses the contiguous Vorbis setup packet and joins its independently
// reconstructed codebook, floor, residue, mapping, and mode body parsers.
// Packet input includes the 0x05 + "vorbis" signature. The native firmware
// uses linked fragments; this adapter currently accepts contiguous bytes.
[[nodiscard]] inline std::optional<VorbisSetupHeader> parse_vorbis_setup_header(
    std::span<const std::uint8_t> packet,
    std::uint8_t channel_count) {
    constexpr std::string_view signature = "vorbis";
    if (packet.size() < signature.size() + 1U || packet[0] != 5 || channel_count == 0)
        return std::nullopt;
    for (std::size_t i = 0; i < signature.size(); ++i) {
        if (packet[i + 1U] != static_cast<std::uint8_t>(signature[i])) return std::nullopt;
    }

    detail::VorbisSetupBitCursor cursor(packet, 7U * 8U);
    const auto start_offset = cursor.offset();
    std::uint32_t value = 0;
    VorbisSetupHeader result;

    if (!cursor.read(8, value)) return std::nullopt;
    const auto codebook_count = value + 1U;
    result.decoder.codebooks.reserve(codebook_count);
    std::vector<std::uint16_t> codebook_dimensions;
    codebook_dimensions.reserve(codebook_count);
    for (std::uint32_t i = 0; i < codebook_count; ++i) {
        auto tail = cursor.remaining_rebased();
        auto codebook = parse_vorbis_codebook_setup(tail);
        if (!codebook || !cursor.skip(codebook->bits_consumed)) {
            return std::nullopt;
        }
        codebook_dimensions.push_back(codebook->dimensions);
        result.decoder.codebooks.push_back(std::move(*codebook));
    }

    if (!cursor.read(6, value)) return std::nullopt;
    const auto time_count = value + 1U;
    result.time_configuration_count = static_cast<std::uint8_t>(time_count);
    for (std::uint32_t i = 0; i < time_count; ++i) {
        if (!cursor.read(16, value) || value != 0) return std::nullopt;
    }

    if (!cursor.read(6, value)) return std::nullopt;
    const auto floor_count = value + 1U;
    result.decoder.floors.reserve(floor_count);
    for (std::uint32_t i = 0; i < floor_count; ++i) {
        std::uint32_t floor_type = 0;
        if (!cursor.read(16, floor_type)) return std::nullopt;
        auto tail = cursor.remaining_rebased();
        if (floor_type == 0) {
            auto floor = parse_vorbis_floor0_setup(tail, codebook_dimensions);
            if (!floor || !cursor.skip(floor->bits_consumed)) return std::nullopt;
            result.decoder.floors.emplace_back(std::move(*floor));
        } else if (floor_type == 1) {
            auto floor = parse_vorbis_floor1_setup(
                tail, static_cast<std::uint16_t>(codebook_count));
            if (!floor || !cursor.skip(floor->bits_consumed)) return std::nullopt;
            result.decoder.floors.emplace_back(std::move(*floor));
        } else {
            return std::nullopt;
        }
    }

    if (!cursor.read(6, value)) return std::nullopt;
    const auto residue_count = value + 1U;
    result.decoder.residues.reserve(residue_count);
    for (std::uint32_t i = 0; i < residue_count; ++i) {
        auto tail = cursor.remaining_rebased();
        auto residue = parse_vorbis_residue_setup(
            tail, static_cast<std::uint16_t>(codebook_count));
        if (!residue || !cursor.skip(residue->bits_consumed)) return std::nullopt;
        result.decoder.residues.push_back(std::move(*residue));
    }

    if (!cursor.read(6, value)) return std::nullopt;
    const auto mapping_count = value + 1U;
    result.decoder.mappings.reserve(mapping_count);
    for (std::uint32_t i = 0; i < mapping_count; ++i) {
        std::uint32_t mapping_type = 0;
        if (!cursor.read(16, mapping_type) || mapping_type != 0) return std::nullopt;
        auto tail = cursor.remaining_rebased();
        auto mapping = parse_vorbis_mapping0(
            tail, channel_count, static_cast<std::uint16_t>(floor_count),
            static_cast<std::uint16_t>(residue_count));
        if (!mapping || !cursor.skip(mapping->bits_consumed)) return std::nullopt;
        for (const auto& submap : mapping->submaps) {
            if (submap.time_configuration >= time_count) return std::nullopt;
        }
        result.decoder.mappings.push_back(std::move(*mapping));
    }

    auto mode_offset = cursor.offset();
    const auto modes = parse_vorbis_mode_setup(
        packet, mode_offset, static_cast<std::uint16_t>(mapping_count));
    if (!modes) return std::nullopt;
    if (!cursor.skip(mode_offset - cursor.offset())) return std::nullopt;
    result.decoder.mode_setup = *modes;
    result.bits_consumed = cursor.offset() - start_offset;
    return result;
}

} // namespace echo::audio
