#include "echo/audio/vorbis_codebook_callbacks.hpp"

namespace echo::audio {

VorbisFloor0VectorDecode make_vorbis_floor0_codebook_decoder(
    std::span<const VorbisCodebookSetup> codebooks) {
    return [codebooks](std::uint8_t book, std::uint16_t dimension,
                       std::span<const std::uint8_t> packet,
                       std::size_t& bit_offset) -> std::optional<std::vector<std::int32_t>> {
        if (book >= codebooks.size() || codebooks[book].dimensions != dimension) {
            return std::nullopt;
        }
        return decode_vorbis_codebook_vector(codebooks[book], packet, bit_offset, -24);
    };
}

VorbisFloorCodebookDecode make_vorbis_floor1_codebook_decoder(
    std::span<const VorbisCodebookSetup> codebooks) {
    return [codebooks](std::uint8_t book, std::span<const std::uint8_t> packet,
                       std::size_t& bit_offset) -> std::optional<std::int32_t> {
        if (book >= codebooks.size()) return std::nullopt;
        return decode_vorbis_codebook_scalar(codebooks[book], packet, bit_offset);
    };
}

} // namespace echo::audio
