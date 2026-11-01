#include "echo/audio/mpeg_huffman_lookup.hpp"
#include "echo/audio/module73_layer3_requantize.hpp"

#include <cstddef>
#include <cstdint>
#include <array>

namespace echo::audio {

namespace {

#include "module73_mpeg_huffman_tables.inc"
#include "module73_mpeg_count1_tables.inc"

constexpr std::span<const std::uint16_t> as_span(
    const auto& table) noexcept {
    return {table.data(), table.size()};
}

const std::array<Module73MpegHuffmanCodebook, 32> kModule73Codebooks{{
    {0x0102b43cU, 0, 0, as_span(tree_102b43c)},
    {0x0102b43eU, 0, 3, as_span(tree_102b43e)},
    {0x0102b44eU, 0, 3, as_span(tree_102b44e)},
    {0x0102b46eU, 0, 3, as_span(tree_102b46e)},
    {0, 0, 0, {}},
    {0x0102b48eU, 0, 3, as_span(tree_102b48e)},
    {0x0102b4c2U, 0, 4, as_span(tree_102b4c2)},
    {0x0102b4faU, 0, 4, as_span(tree_102b4fa)},
    {0x0102b57aU, 0, 4, as_span(tree_102b57a)},
    {0x0102b5feU, 0, 4, as_span(tree_102b5fe)},
    {0x0102b66aU, 0, 4, as_span(tree_102b66a)},
    {0x0102b736U, 0, 4, as_span(tree_102b736)},
    {0x0102b806U, 0, 4, as_span(tree_102b806)},
    {0x0102b8c6U, 0, 4, as_span(tree_102b8c6)},
    {0, 0, 0, {}},
    {0x0102bbe2U, 0, 4, as_span(tree_102bbe2)},
    {0x0102bedaU, 1, 4, as_span(tree_102beda)},
    {0x0102bedaU, 2, 4, as_span(tree_102beda)},
    {0x0102bedaU, 3, 4, as_span(tree_102beda)},
    {0x0102bedaU, 4, 4, as_span(tree_102beda)},
    {0x0102bedaU, 6, 4, as_span(tree_102beda)},
    {0x0102bedaU, 8, 4, as_span(tree_102beda)},
    {0x0102bedaU, 10, 4, as_span(tree_102beda)},
    {0x0102bedaU, 13, 4, as_span(tree_102beda)},
    {0x0102c1feU, 4, 4, as_span(tree_102c1fe)},
    {0x0102c1feU, 5, 4, as_span(tree_102c1fe)},
    {0x0102c1feU, 6, 4, as_span(tree_102c1fe)},
    {0x0102c1feU, 7, 4, as_span(tree_102c1fe)},
    {0x0102c1feU, 8, 4, as_span(tree_102c1fe)},
    {0x0102c1feU, 9, 4, as_span(tree_102c1fe)},
    {0x0102c1feU, 11, 4, as_span(tree_102c1fe)},
    {0x0102c1feU, 13, 4, as_span(tree_102c1fe)},
}};

} // namespace

std::optional<Module73MpegHuffmanCodebook>
module73_mpeg_huffman_codebook(const unsigned index) noexcept {
    if (index >= kModule73Codebooks.size()) return std::nullopt;
    return kModule73Codebooks[index];
}

std::optional<MpegHuffmanLookupResult> decode_module73_mpeg_huffman_symbol(
    echo::bitstream::MsbBitReader& bits,
    const std::span<const std::uint16_t> table,
    const unsigned root_bits) noexcept {
    if (root_bits == 0 || root_bits > 16) return std::nullopt;
    // The firmware looks ahead by root_bits but does not necessarily consume
    // them: root entries can duplicate a shorter leaf across several slots.
    // Decode against a copy, then consume the encoded code length once known.
    auto lookahead = bits;
    const auto root = lookahead.read(root_bits);
    if (!root || *root >= table.size()) return std::nullopt;

    std::size_t index = *root;
    unsigned consumed = 0;
    for (std::size_t step = 0; step < table.size(); ++step) {
        if (index >= table.size()) return std::nullopt;
        const std::uint16_t entry = table[index];
        if ((entry & 1U) != 0) {
            const unsigned leaf_bits = (entry & 0x0fU) >> 1U;
            if (consumed == 0) {
                consumed = leaf_bits;
            } else {
                consumed += leaf_bits;
            }
            if (!bits.read(consumed)) return std::nullopt;
            return MpegHuffmanLookupResult{
                static_cast<std::uint16_t>(entry >> 4U), consumed};
        }

        if (consumed == 0) consumed = root_bits;
        const unsigned branch_bits = (entry & 0x0fU) >> 1U;
        if (branch_bits == 0) return std::nullopt;
        const auto branch = lookahead.read(branch_bits);
        if (!branch) return std::nullopt;
        consumed += branch_bits;
        index = static_cast<std::size_t>(entry >> 4U) + *branch;
    }
    return std::nullopt;
}

std::optional<MpegHuffmanPair> decode_module73_mpeg_huffman_pair(
    echo::bitstream::MsbBitReader& bits,
    const std::span<const std::uint16_t> table,
    const unsigned root_bits,
    const unsigned linbits,
    const std::int32_t exponent) noexcept {
    return decode_module73_mpeg_huffman_pair(
        bits, table, root_bits, linbits, exponent, exponent);
}

std::optional<MpegHuffmanPair> decode_module73_mpeg_huffman_pair(
    echo::bitstream::MsbBitReader& bits,
    const std::span<const std::uint16_t> table,
    const unsigned root_bits,
    const unsigned linbits,
    const std::int32_t x_exponent,
    const std::int32_t y_exponent) noexcept {
    if (linbits > 16) return std::nullopt;
    const std::size_t start = bits.bits_consumed();
    const auto decoded = decode_module73_mpeg_huffman_symbol(bits, table, root_bits);
    if (!decoded) return std::nullopt;

    std::uint32_t x = decoded->symbol & 0x0fU;
    std::uint32_t y = (decoded->symbol >> 4U) & 0x0fU;
    if (x == 0x0fU && linbits != 0) {
        const auto extra = bits.read(linbits);
        if (!extra) return std::nullopt;
        x += *extra;
    }
    bool x_negative = false;
    if (x != 0) {
        const auto sign = bits.read(1);
        if (!sign) return std::nullopt;
        x_negative = *sign != 0;
    }

    if (y == 0x0fU && linbits != 0) {
        const auto extra = bits.read(linbits);
        if (!extra) return std::nullopt;
        y += *extra;
    }
    bool y_negative = false;
    if (y != 0) {
        const auto sign = bits.read(1);
        if (!sign) return std::nullopt;
        y_negative = *sign != 0;
    }

    const auto scaled_x = module73_layer3_requantize(x, x_exponent);
    const auto scaled_y = module73_layer3_requantize(y, y_exponent);
    if (!scaled_x || !scaled_y) return std::nullopt;
    const auto signed_x = static_cast<std::int32_t>(*scaled_x);
    const auto signed_y = static_cast<std::int32_t>(*scaled_y);
    const std::size_t bit_delta = bits.bits_consumed() - start;
    if (bit_delta > 0xffffffffU) return std::nullopt;
    return MpegHuffmanPair{x_negative ? -signed_x : signed_x,
                           y_negative ? -signed_y : signed_y,
                           static_cast<unsigned>(bit_delta)};
}

std::optional<MpegHuffmanPair> decode_module73_mpeg_huffman_codebook_pair(
    echo::bitstream::MsbBitReader& bits,
    const unsigned codebook_index,
    const std::int32_t exponent) noexcept {
    return decode_module73_mpeg_huffman_codebook_pair(
        bits, codebook_index, exponent, exponent);
}

std::optional<MpegHuffmanPair> decode_module73_mpeg_huffman_codebook_pair(
    echo::bitstream::MsbBitReader& bits,
    const unsigned codebook_index,
    const std::int32_t x_exponent,
    const std::int32_t y_exponent) noexcept {
    if (codebook_index == 0) return MpegHuffmanPair{};
    const auto codebook = module73_mpeg_huffman_codebook(codebook_index);
    if (!codebook || !codebook->decodable()) return std::nullopt;
    return decode_module73_mpeg_huffman_pair(
        bits, codebook->lookup_table, codebook->root_bits,
        codebook->linbits, x_exponent, y_exponent);
}

std::optional<MpegHuffmanCount1> decode_module73_mpeg_huffman_count1(
    echo::bitstream::MsbBitReader& bits,
    const unsigned count1_table_select,
    const std::int32_t exponent) noexcept {
    return decode_module73_mpeg_huffman_count1(
        bits, count1_table_select,
        std::array<std::int32_t, 4>{exponent, exponent, exponent, exponent});
}

std::optional<MpegHuffmanCount1> decode_module73_mpeg_huffman_count1(
    echo::bitstream::MsbBitReader& bits,
    const unsigned count1_table_select,
    const std::array<std::int32_t, 4>& coefficient_exponents) noexcept {
    if (count1_table_select > 1) return std::nullopt;
    const auto table = count1_table_select == 0
        ? as_span(tree_102b3e4)
        : as_span(tree_102b41c);
    const std::size_t start = bits.bits_consumed();
    const auto decoded = decode_module73_mpeg_huffman_symbol(bits, table, 4);
    if (!decoded) return std::nullopt;

    MpegHuffmanCount1 result{};
    for (unsigned i = 0; i < result.values.size(); ++i) {
        if ((decoded->symbol & (1U << i)) == 0) continue;
        const auto sign = bits.read(1);
        if (!sign) return std::nullopt;
        const auto magnitude = module73_layer3_requantize(
            1, coefficient_exponents[i]);
        if (!magnitude) return std::nullopt;
        const auto value = static_cast<std::int32_t>(*magnitude);
        result.values[i] = *sign != 0 ? -value : value;
    }
    const std::size_t bit_delta = bits.bits_consumed() - start;
    if (bit_delta > 0xffffffffU) return std::nullopt;
    result.bits_consumed = static_cast<unsigned>(bit_delta);
    return result;
}

} // namespace echo::audio
