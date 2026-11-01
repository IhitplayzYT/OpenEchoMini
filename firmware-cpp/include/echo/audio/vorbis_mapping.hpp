#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace echo::audio {

struct VorbisMappingSubmap {
    std::uint8_t time_configuration{};
    std::uint8_t floor_index{};
    std::uint8_t residue_index{};
};

struct VorbisMapping0 {
    std::vector<std::uint8_t> channel_mux;
    std::vector<std::pair<std::uint8_t, std::uint8_t>> coupling_steps;
    std::vector<VorbisMappingSubmap> submaps;
    std::size_t bits_consumed{};
};

// Parses the type-0 mapping body after its 16-bit mapping-type selector.
// This mirrors module 77's LSB-first bit fields and validates channel,
// floor, and residue indices against the setup counts supplied by the caller.
[[nodiscard]] std::optional<VorbisMapping0> parse_vorbis_mapping0(
    std::span<const std::uint8_t> mapping_bits,
    std::uint8_t channel_count,
    std::uint16_t floor_count,
    std::uint16_t residue_count);

} // namespace echo::audio
