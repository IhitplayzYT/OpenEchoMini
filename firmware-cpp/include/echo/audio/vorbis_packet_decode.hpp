#pragma once

#include "echo/audio/vorbis_inverse_transform.hpp"
#include "echo/audio/vorbis_mapping_packet.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace echo::audio {

using VorbisChannelFloorDecode = std::function<std::optional<bool>(
    std::uint8_t channel, std::uint8_t floor_index,
    std::span<const std::uint8_t> packet, std::size_t& bit_offset)>;

using VorbisSubmapResidueDecode = std::function<bool(
    std::uint8_t residue_index, const VorbisSubmapPacketPlan& group,
    std::span<const std::uint8_t> packet, std::size_t& bit_offset,
    std::span<std::vector<std::int32_t>> channel_spectra)>;

using VorbisChannelFloorRender = std::function<bool(
    std::uint8_t channel, std::uint8_t floor_index,
    std::span<std::int32_t> spectrum)>;

struct VorbisPacketDecodeResult {
    VorbisMappingPacketPlan mapping_plan;
    std::size_t bits_consumed{};
};

// Runs the recovered post-mode audio packet sequence. The caller parses the
// packet mode first, chooses its mapping/block size, and supplies callbacks
// that bind parsed floor/residue setup to the existing component decoders.
// Floor callbacks share bit_offset so codebook vectors, scalar books, and
// residue stages all consume the same packet bitstream.
[[nodiscard]] inline std::optional<VorbisPacketDecodeResult>
decode_vorbis_mapping_packet(
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    const VorbisMapping0& mapping,
    std::uint32_t block_size,
    std::span<std::vector<std::int32_t>> channel_spectra,
    const VorbisChannelFloorDecode& decode_floor,
    const VorbisSubmapResidueDecode& decode_residue,
    const VorbisChannelFloorRender& render_floor) {
    if (!decode_floor || !decode_residue || !render_floor ||
        bit_offset > packet.size() * 8 ||
        channel_spectra.size() != mapping.channel_mux.size()) {
        return std::nullopt;
    }
    const auto transform = plan_vorbis_inverse_transform(block_size);
    if (!transform || channel_spectra.empty()) return std::nullopt;
    for (const auto& spectrum : channel_spectra) {
        if (spectrum.size() < transform->half_block) return std::nullopt;
    }

    const auto start_offset = bit_offset;
    std::vector<std::uint8_t> floor_present(channel_spectra.size());
    for (std::size_t channel = 0; channel < channel_spectra.size(); ++channel) {
        const auto mux = mapping.channel_mux[channel];
        if (mux >= mapping.submaps.size()) return std::nullopt;
        const auto decoded = decode_floor(
            static_cast<std::uint8_t>(channel), mapping.submaps[mux].floor_index,
            packet, bit_offset);
        if (!decoded || bit_offset > packet.size() * 8) return std::nullopt;
        floor_present[channel] = *decoded ? 1U : 0U;
    }

    auto plan = plan_vorbis_mapping_packet(mapping, floor_present);
    if (!plan) return std::nullopt;
    for (const auto& group : plan->residue_submaps) {
        if (!decode_residue(group.residue_index, group, packet, bit_offset,
                            channel_spectra) ||
            bit_offset > packet.size() * 8) {
            return std::nullopt;
        }
    }

    if (!apply_vorbis_inverse_coupling(mapping, channel_spectra,
                                       transform->half_block)) {
        return std::nullopt;
    }
    for (std::size_t channel = 0; channel < channel_spectra.size(); ++channel) {
        const auto mux = mapping.channel_mux[channel];
        const auto floor_index = mapping.submaps[mux].floor_index;
        auto spectrum = std::span<std::int32_t>(channel_spectra[channel]).first(
            transform->half_block);
        if (!render_floor(static_cast<std::uint8_t>(channel), floor_index, spectrum))
            return std::nullopt;
    }
    for (auto& channel_spectrum : channel_spectra) {
        auto spectrum = std::span<std::int32_t>(channel_spectrum).first(
            transform->half_block);
        if (!apply_vorbis_inverse_transform_stages(*transform, spectrum))
            return std::nullopt;
    }

    return VorbisPacketDecodeResult{std::move(*plan), bit_offset - start_offset};
}

} // namespace echo::audio
