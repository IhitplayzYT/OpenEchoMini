#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

struct VorbisCodebookNode {
    std::int32_t child[2]{-1, -1};
    std::int32_t symbol{-1};
};

struct VorbisCodebookSetup {
    std::uint16_t dimensions{};
    std::uint32_t entries{};
    std::vector<std::uint8_t> codeword_lengths; // zero means a sparse absent entry
    std::uint8_t lookup_type{};
    std::uint32_t lookup_values{};
    std::uint32_t minimum_value_bits{};
    std::uint32_t delta_value_bits{};
    std::int32_t minimum_mantissa{};
    std::int32_t minimum_exponent{};
    std::int32_t delta_mantissa{};
    std::int32_t delta_exponent{};
    std::uint8_t value_bits{};
    bool sequence{};
    std::vector<std::uint16_t> lookup_multiplicands;
    std::vector<VorbisCodebookNode> decode_tree;
    std::size_t bits_consumed{};
};

[[nodiscard]] std::optional<VorbisCodebookSetup> parse_vorbis_codebook_setup(
    std::span<const std::uint8_t> setup_bits,
    std::size_t max_entries = 1U << 20,
    std::size_t max_lookup_values = 1U << 22,
    std::size_t max_decode_nodes = 1U << 22);

[[nodiscard]] std::optional<std::uint32_t> decode_vorbis_codebook_symbol(
    const VorbisCodebookSetup& setup,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset);

// Returns one codebook vector in the signed fixed-point exponent requested by
// the caller. The firmware Floor-0 path requests exponent -24.
[[nodiscard]] std::optional<std::vector<std::int32_t>> decode_vorbis_codebook_vector(
    const VorbisCodebookSetup& setup,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    std::int32_t target_exponent = -24);

// Floor-1 codebooks consume a scalar symbol. Lookup codebooks are accepted
// here only when their vector dimension is one.
[[nodiscard]] std::optional<std::int32_t> decode_vorbis_codebook_scalar(
    const VorbisCodebookSetup& setup,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset);

} // namespace echo::audio
