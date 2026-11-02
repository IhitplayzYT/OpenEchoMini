#pragma once

#include "echo/audio/vorbis_codebook.hpp"
#include "echo/audio/vorbis_floor0.hpp"
#include "echo/audio/vorbis_floor1.hpp"

#include <span>

namespace echo::audio {

[[nodiscard]] VorbisFloor0VectorDecode make_vorbis_floor0_codebook_decoder(
    std::span<const VorbisCodebookSetup> codebooks);

[[nodiscard]] VorbisFloorCodebookDecode make_vorbis_floor1_codebook_decoder(
    std::span<const VorbisCodebookSetup> codebooks);

} // namespace echo::audio
