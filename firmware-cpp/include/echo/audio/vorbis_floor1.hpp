#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

struct VorbisFloor1Class {
    std::uint8_t dimensions{};
    std::uint8_t subclass_bits{};
    std::optional<std::uint8_t> masterbook;
    // Size is 1 << subclass_bits, including one entry when the width is zero.
    // 255 denotes an absent subclass book, matching the firmware's sentinel.
    std::vector<std::uint8_t> subclass_books;
};

struct VorbisFloor1Setup {
    std::uint16_t y_range{};
    std::uint8_t multiplier{};
    std::uint8_t range_bits{};
    std::vector<std::uint8_t> partition_classes;
    std::vector<VorbisFloor1Class> classes;
    // Post 0 and post 1 are the two endpoint posts.
    std::vector<std::uint16_t> x_list;
    // Stable order used by the firmware when drawing posts from low x to high x.
    std::vector<std::uint8_t> sorted_post_order;
    std::vector<std::uint8_t> low_neighbor;
    std::vector<std::uint8_t> high_neighbor;
    std::size_t bits_consumed{};
};

struct VorbisFloor1Packet {
    bool present{};
    std::vector<std::uint16_t> y;
    std::vector<bool> active;
    std::size_t bits_consumed{};
};

// The callback decodes one scalar from a Vorbis codebook and advances bit_offset.
// A missing callback result or invalid bit offset aborts the packet decode.
using VorbisFloorCodebookDecode = std::function<std::optional<std::int32_t>(
    std::uint8_t book, std::span<const std::uint8_t> packet, std::size_t& bit_offset)>;

// Parses one Floor-1 setup body and validates every master/subclass book index
// against the number of codebooks already parsed from the setup header.
[[nodiscard]] std::optional<VorbisFloor1Setup> parse_vorbis_floor1_setup(
    std::span<const std::uint8_t> setup_bits,
    std::uint16_t codebook_count);

// Decodes floor-1 packet posts using already parsed setup data. The caller owns
// codebook decoding and floor curve rendering; this routine reproduces presence,
// class/subclass selection, and predicted-post unwrapping.
[[nodiscard]] std::optional<VorbisFloor1Packet> decode_vorbis_floor1_packet(
    std::span<const std::uint8_t> packet,
    const VorbisFloor1Setup& setup,
    const VorbisFloorCodebookDecode& decode_scalar);

} // namespace echo::audio
