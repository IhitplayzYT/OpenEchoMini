#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

struct VorbisFloor0Setup {
    std::uint16_t order{};
    std::uint16_t rate{};
    std::uint16_t bark_map_size{};
    std::uint8_t amplitude_bits{};
    std::uint16_t amplitude_offset{};
    std::vector<std::uint8_t> books;
    std::vector<std::uint16_t> book_dimensions;
    std::size_t bits_consumed{};
};

struct VorbisFloor0Packet {
    bool present{};
    std::uint32_t amplitude{};
    std::uint8_t book_index{};
    std::vector<std::int32_t> lsp_values;
    std::uint32_t gain_numerator{};
    std::size_t bits_consumed{};
};

using VorbisFloor0VectorDecode = std::function<std::optional<std::vector<std::int32_t>>(
    std::uint8_t book, std::uint16_t dimension,
    std::span<const std::uint8_t> packet, std::size_t& bit_offset)>;

// Parses a Floor-0 setup body and resolves its book dimensions from the
// codebooks already decoded from the setup header.
[[nodiscard]] std::optional<VorbisFloor0Setup> parse_vorbis_floor0_setup(
    std::span<const std::uint8_t> setup_bits,
    std::span<const std::uint16_t> codebook_dimensions);

// Decodes Floor-0 amplitude, book selector, and cumulative LSP vectors. The
// caller supplies codebook vector decoding and later performs bark mapping and
// floor curve synthesis.
[[nodiscard]] std::optional<VorbisFloor0Packet> decode_vorbis_floor0_packet(
    std::span<const std::uint8_t> packet,
    const VorbisFloor0Setup& setup,
    const VorbisFloor0VectorDecode& decode_vector);

// Continues decoding at a shared packet bit cursor, allowing the floor to
// follow the packet-mode prefix without copying or rebasing packet bytes.
[[nodiscard]] std::optional<VorbisFloor0Packet> decode_vorbis_floor0_packet(
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    const VorbisFloor0Setup& setup,
    const VorbisFloor0VectorDecode& decode_vector);

} // namespace echo::audio
