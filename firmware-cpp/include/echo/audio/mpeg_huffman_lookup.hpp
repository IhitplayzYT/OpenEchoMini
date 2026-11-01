#pragma once

#include "echo/bitstream/msb_bit_reader.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct MpegHuffmanLookupResult {
    std::uint16_t symbol{};
    unsigned bits_consumed{};
};

struct MpegHuffmanPair {
    std::int32_t x{};
    std::int32_t y{};
    unsigned bits_consumed{};
};

struct MpegHuffmanCount1 {
    std::array<std::int32_t, 4> values{};
    unsigned bits_consumed{};
};

struct Module73MpegHuffmanCodebook {
    std::uint32_t firmware_table_address{};
    unsigned linbits{};
    unsigned root_bits{};
    std::span<const std::uint16_t> lookup_table{};

    [[nodiscard]] bool decodable() const noexcept {
        return root_bits != 0 && !lookup_table.empty();
    }
};

// Returns one of the 32 descriptor slots from 0x0102c50c. Reserved and
// zero-codebook slots are represented with an empty lookup span.
[[nodiscard]] std::optional<Module73MpegHuffmanCodebook>
module73_mpeg_huffman_codebook(unsigned index) noexcept;

// Decodes one symbol from the firmware's 16-bit root/branch lookup format.
[[nodiscard]] std::optional<MpegHuffmanLookupResult>
decode_module73_mpeg_huffman_symbol(
    echo::bitstream::MsbBitReader& bits,
    std::span<const std::uint16_t> table,
    unsigned root_bits) noexcept;

// Adds per-component linbits and sign bits, then applies the fixed-point
// magnitude table. Descriptor selection remains caller-owned in this overload.
[[nodiscard]] std::optional<MpegHuffmanPair> decode_module73_mpeg_huffman_pair(
    echo::bitstream::MsbBitReader& bits,
    std::span<const std::uint16_t> table,
    unsigned root_bits,
    unsigned linbits,
    std::int32_t exponent) noexcept;

[[nodiscard]] std::optional<MpegHuffmanPair> decode_module73_mpeg_huffman_pair(
    echo::bitstream::MsbBitReader& bits,
    std::span<const std::uint16_t> table,
    unsigned root_bits,
    unsigned linbits,
    std::int32_t x_exponent,
    std::int32_t y_exponent) noexcept;

// Selects the observed module 73 descriptor by side-information codebook index
// and applies the matching root width and linbits.
[[nodiscard]] std::optional<MpegHuffmanPair>
decode_module73_mpeg_huffman_codebook_pair(
    echo::bitstream::MsbBitReader& bits,
    unsigned codebook_index,
    std::int32_t exponent) noexcept;

[[nodiscard]] std::optional<MpegHuffmanPair>
decode_module73_mpeg_huffman_codebook_pair(
    echo::bitstream::MsbBitReader& bits,
    unsigned codebook_index,
    std::int32_t x_exponent,
    std::int32_t y_exponent) noexcept;

// Decodes one count1 quadruple using the firmware's two tables selected by
// side-information count1table_select, then reads signs for nonzero values.
[[nodiscard]] std::optional<MpegHuffmanCount1>
decode_module73_mpeg_huffman_count1(
    echo::bitstream::MsbBitReader& bits,
    unsigned count1_table_select,
    std::int32_t exponent) noexcept;

[[nodiscard]] std::optional<MpegHuffmanCount1>
decode_module73_mpeg_huffman_count1(
    echo::bitstream::MsbBitReader& bits,
    unsigned count1_table_select,
    const std::array<std::int32_t, 4>& coefficient_exponents) noexcept;

} // namespace echo::audio
