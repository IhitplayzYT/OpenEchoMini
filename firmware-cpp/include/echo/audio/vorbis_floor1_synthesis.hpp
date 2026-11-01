#pragma once

#include "echo/audio/vorbis_floor1.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

// Applies the active Floor-1 posts to one half-block of residue spectrum.
// inverse_db_table holds the firmware's fixed-point inverse-dB gains; its
// contents are caller supplied because the table bytes are not yet verified.
[[nodiscard]] std::optional<std::vector<std::int32_t>> render_vorbis_floor1_spectrum(
    std::span<const std::int32_t> residue_spectrum,
    const VorbisFloor1Setup& setup,
    const VorbisFloor1Packet& floor,
    std::uint8_t multiplier,
    std::span<const std::int32_t> inverse_db_table);

// Uses the verified module 77 inverse-dB table.
[[nodiscard]] std::optional<std::vector<std::int32_t>> render_vorbis_floor1_spectrum(
    std::span<const std::int32_t> residue_spectrum,
    const VorbisFloor1Setup& setup,
    const VorbisFloor1Packet& floor,
    std::uint8_t multiplier);

} // namespace echo::audio
