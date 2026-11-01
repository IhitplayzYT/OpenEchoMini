#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "echo/bitstream/msb_bit_reader.hpp"

namespace echo::image {

struct JpegHuffmanTableSpec {
    std::uint8_t selector{};
    std::uint8_t table_index{};
    std::array<std::uint8_t, 16> code_counts{};
    std::vector<std::uint8_t> symbols;
};

class JpegHuffmanDecoder {
public:
    [[nodiscard]] std::optional<std::uint8_t> decode(
        bitstream::MsbBitReader& reader) const noexcept;

private:
    friend std::optional<JpegHuffmanDecoder> build_jpeg_huffman_decoder(
        const JpegHuffmanTableSpec& spec);

    std::array<std::uint32_t, 16> first_codes_{};
    std::array<std::size_t, 16> first_symbols_{};
    std::array<std::uint8_t, 16> code_counts_{};
    std::vector<std::uint8_t> symbols_;
};

// Parses the length-prefixed table records observed in module 19. It validates
// the 16 code-length counts, symbol bounds, and the firmware's eight-slot
// table index. It does not build decode lookup tables.
[[nodiscard]] std::optional<std::vector<JpegHuffmanTableSpec>>
parse_jpeg_huffman_table_specs(std::span<const std::uint8_t> bytes);

// Builds canonical codes from the parsed length counts. Oversubscribed or
// inconsistent tables fail; incomplete tables are allowed.
[[nodiscard]] std::optional<JpegHuffmanDecoder> build_jpeg_huffman_decoder(
    const JpegHuffmanTableSpec& spec);

} // namespace echo::image
