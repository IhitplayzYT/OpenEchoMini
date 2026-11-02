#pragma once

#include "echo/audio/vorbis_codebook_callbacks.hpp"
#include "echo/audio/vorbis_floor0_synthesis.hpp"
#include "echo/audio/vorbis_floor1_synthesis.hpp"
#include "echo/audio/vorbis_identification.hpp"
#include "echo/audio/vorbis_mode_setup.hpp"
#include "echo/audio/vorbis_packet_decode.hpp"
#include "echo/audio/vorbis_residue.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace echo::audio {

using VorbisFloorSetup = std::variant<VorbisFloor0Setup, VorbisFloor1Setup>;

struct VorbisAudioSetupContext {
    std::vector<VorbisCodebookSetup> codebooks;
    std::vector<VorbisFloorSetup> floors;
    std::vector<VorbisResidueSetup> residues;
    std::vector<VorbisMapping0> mappings;
    VorbisModeSetup mode_setup;
};

struct VorbisAudioPacketResult {
    VorbisPacketMode mode;
    VorbisMappingPacketPlan mapping_plan;
    std::uint32_t block_size{};
    std::size_t bits_consumed{};
};

// Executes one complete audio packet from its mode prefix through per-channel
// inverse transform. Setup records are caller supplied from the corresponding
// header parsers; this routine joins their indices to the packet callbacks.
// The resulting half-block spectra still require windowing/overlap before PCM.
[[nodiscard]] inline std::optional<VorbisAudioPacketResult>
decode_vorbis_audio_packet(
    std::span<const std::uint8_t> packet,
    const VorbisIdentificationHeader& identification,
    const VorbisAudioSetupContext& setup,
    std::span<std::vector<std::int32_t>> channel_spectra) {
    if (identification.channels == 0 ||
        channel_spectra.size() != identification.channels ||
        setup.mappings.empty() || setup.mappings.size() > 256 ||
        setup.floors.empty() || setup.floors.size() > 256 ||
        setup.residues.empty() || setup.residues.size() > 256 ||
        setup.codebooks.empty() || setup.mode_setup.modes.empty()) {
        return std::nullopt;
    }

    std::size_t bit_offset = 0;
    const auto mode = decode_vorbis_packet_mode(
        packet, bit_offset, setup.mode_setup.modes,
        static_cast<std::uint16_t>(setup.mappings.size()));
    if (!mode || mode->mapping_index >= setup.mappings.size()) return std::nullopt;
    const auto block_size = mode->long_block ? identification.large_block_size
                                             : identification.small_block_size;
    const auto mapping_index = mode->mapping_index;
    const auto& mapping = setup.mappings[mapping_index];
    const auto floor0_decode = make_vorbis_floor0_codebook_decoder(setup.codebooks);
    const auto floor1_decode = make_vorbis_floor1_codebook_decoder(setup.codebooks);
    using DecodedFloor = std::variant<std::monostate, VorbisFloor0Packet,
                                      VorbisFloor1Packet>;
    std::vector<DecodedFloor> decoded_floors(identification.channels);

    const VorbisChannelFloorDecode decode_floor =
        [&](std::uint8_t channel, std::uint8_t floor_index,
            std::span<const std::uint8_t> bytes,
            std::size_t& cursor) -> std::optional<bool> {
            if (channel >= decoded_floors.size() || floor_index >= setup.floors.size())
                return std::nullopt;
            const auto& floor_setup = setup.floors[floor_index];
            if (const auto* floor0 = std::get_if<VorbisFloor0Setup>(&floor_setup)) {
                const auto decoded = decode_vorbis_floor0_packet(
                    bytes, cursor, *floor0, floor0_decode);
                if (!decoded) return std::nullopt;
                const auto present = decoded->present;
                decoded_floors[channel] = *decoded;
                return present;
            }
            const auto* floor1 = std::get_if<VorbisFloor1Setup>(&floor_setup);
            if (!floor1) return std::nullopt;
            const auto decoded = decode_vorbis_floor1_packet(
                bytes, cursor, *floor1, floor1_decode);
            if (!decoded) return std::nullopt;
            const auto present = decoded->present;
            decoded_floors[channel] = *decoded;
            return present;
        };

    const VorbisSubmapResidueDecode decode_residue =
        [&](std::uint8_t residue_index, const VorbisSubmapPacketPlan& group,
            std::span<const std::uint8_t> bytes, std::size_t& cursor,
            std::span<std::vector<std::int32_t>> spectra) -> bool {
            if (residue_index >= setup.residues.size() ||
                group.channels.size() != group.channel_active.size() ||
                group.channels.empty()) {
                return false;
            }
            std::vector<std::vector<std::int32_t>> selected_spectra;
            selected_spectra.reserve(group.channels.size());
            for (const auto channel : group.channels) {
                if (channel >= spectra.size()) return false;
                selected_spectra.push_back(spectra[channel]);
            }

            const auto& residue = setup.residues[residue_index];
            const auto ok = residue.type == 2
                ? decode_vorbis_residue_type2(residue, setup.codebooks, bytes, cursor,
                                              group.channel_active, selected_spectra)
                : decode_vorbis_residue_channels(residue, setup.codebooks, bytes, cursor,
                                                 group.channel_active, selected_spectra);
            if (!ok) return false;
            for (std::size_t i = 0; i < group.channels.size(); ++i)
                spectra[group.channels[i]] = std::move(selected_spectra[i]);
            return true;
        };

    const VorbisChannelFloorRender render_floor =
        [&](std::uint8_t channel, std::uint8_t floor_index,
            std::span<std::int32_t> spectrum) -> bool {
            if (channel >= decoded_floors.size() || floor_index >= setup.floors.size())
                return false;
            const auto& floor_setup = setup.floors[floor_index];
            std::optional<std::vector<std::int32_t>> rendered;
            if (const auto* floor0 = std::get_if<VorbisFloor0Setup>(&floor_setup)) {
                const auto* decoded = std::get_if<VorbisFloor0Packet>(&decoded_floors[channel]);
                if (!decoded) return false;
                rendered = render_vorbis_floor0_spectrum(spectrum, *floor0, *decoded);
            } else if (const auto* floor1 = std::get_if<VorbisFloor1Setup>(&floor_setup)) {
                const auto* decoded = std::get_if<VorbisFloor1Packet>(&decoded_floors[channel]);
                if (!decoded) return false;
                rendered = render_vorbis_floor1_spectrum(
                    spectrum, *floor1, *decoded, floor1->multiplier);
            }
            if (!rendered || rendered->size() != spectrum.size()) return false;
            std::copy(rendered->begin(), rendered->end(), spectrum.begin());
            return true;
        };

    const auto pipeline = decode_vorbis_mapping_packet(
        packet, bit_offset, mapping, block_size, channel_spectra,
        decode_floor, decode_residue, render_floor);
    if (!pipeline) return std::nullopt;
    return VorbisAudioPacketResult{
        *mode, pipeline->mapping_plan, block_size,
        bit_offset};
}

} // namespace echo::audio
