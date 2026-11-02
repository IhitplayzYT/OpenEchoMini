#pragma once

#include "echo/audio/vorbis_codebook.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

struct VorbisResidueClass {
    std::uint8_t cascade{};
    std::array<std::uint8_t, 8> stage_books{0xff, 0xff, 0xff, 0xff,
                                            0xff, 0xff, 0xff, 0xff};
};

struct VorbisResidueSetup {
    std::uint16_t type{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t partition_size{};
    std::uint8_t classifications{};
    std::uint8_t classbook{};
    std::uint8_t stage_count{};
    std::vector<VorbisResidueClass> classes;
    std::size_t bits_consumed{};
};

[[nodiscard]] std::optional<VorbisResidueSetup> parse_vorbis_residue_setup(
    std::span<const std::uint8_t> setup_bits,
    std::uint16_t codebook_count);

// Decodes residue types 0 and 1 into independent channel spectra. Type 2 has
// a distinct channel-interleaved scatter path and is not accepted here.
[[nodiscard]] bool decode_vorbis_residue_channels(
    const VorbisResidueSetup& setup,
    std::span<const VorbisCodebookSetup> codebooks,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    std::span<const std::uint8_t> channel_active,
    std::span<std::vector<std::int32_t>> channel_spectra);

// Type 2 classifies flattened channel-interleaved partitions and distributes
// each stagebook vector across every supplied channel spectrum. As in the
// firmware worker, any active channel enables the coupled residue path.
[[nodiscard]] bool decode_vorbis_residue_type2(
    const VorbisResidueSetup& setup,
    std::span<const VorbisCodebookSetup> codebooks,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    std::span<const std::uint8_t> channel_active,
    std::span<std::vector<std::int32_t>> channel_spectra);

} // namespace echo::audio
