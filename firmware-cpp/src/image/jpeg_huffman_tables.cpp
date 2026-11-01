#include "echo/image/jpeg_huffman_tables.hpp"

#include <utility>

namespace echo::image {

std::optional<std::uint8_t> JpegHuffmanDecoder::decode(
    bitstream::MsbBitReader& reader) const noexcept {
    std::uint32_t code = 0;
    for (std::size_t length = 0; length < code_counts_.size(); ++length) {
        const auto bit = reader.read(1);
        if (!bit) {
            return std::nullopt;
        }
        code = (code << 1U) | *bit;
        if (code >= first_codes_[length]) {
            const auto offset = static_cast<std::size_t>(code - first_codes_[length]);
            if (offset < code_counts_[length]) {
                const auto symbol_index = first_symbols_[length] + offset;
                if (symbol_index < symbols_.size()) {
                    return symbols_[symbol_index];
                }
                return std::nullopt;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::vector<JpegHuffmanTableSpec>> parse_jpeg_huffman_table_specs(
    const std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 2) {
        return std::nullopt;
    }
    const auto declared_length = (static_cast<std::size_t>(bytes[0]) << 8U)
        | static_cast<std::size_t>(bytes[1]);
    if (declared_length < 2 || declared_length > bytes.size()) {
        return std::nullopt;
    }

    std::vector<JpegHuffmanTableSpec> tables;
    std::size_t cursor = 2;
    while (cursor < declared_length) {
        if (declared_length - cursor < 17) {
            return std::nullopt;
        }

        JpegHuffmanTableSpec table{};
        table.selector = bytes[cursor++];
        std::size_t symbol_count = 0;
        for (auto& count : table.code_counts) {
            count = bytes[cursor++];
            symbol_count += count;
        }
        if (symbol_count > 255 || symbol_count > declared_length - cursor) {
            return std::nullopt;
        }

        const auto table_base = (table.selector & 0x10U) != 0 ? 4U : 0U;
        const auto table_index = table_base + (table.selector & 0x0fU);
        if (table_index > 7) {
            return std::nullopt;
        }
        table.table_index = static_cast<std::uint8_t>(table_index);
        table.symbols.assign(bytes.begin() + static_cast<std::ptrdiff_t>(cursor),
                             bytes.begin() + static_cast<std::ptrdiff_t>(cursor + symbol_count));
        cursor += symbol_count;
        tables.push_back(std::move(table));
    }
    return tables;
}

std::optional<JpegHuffmanDecoder> build_jpeg_huffman_decoder(
    const JpegHuffmanTableSpec& spec) {
    std::size_t total_symbols = 0;
    for (const auto count : spec.code_counts) {
        total_symbols += count;
    }
    if (total_symbols != spec.symbols.size()) {
        return std::nullopt;
    }

    JpegHuffmanDecoder decoder;
    decoder.symbols_ = spec.symbols;
    std::uint32_t next_code = 0;
    std::size_t symbol_index = 0;
    for (std::size_t i = 0; i < spec.code_counts.size(); ++i) {
        const auto bit_length = static_cast<unsigned>(i + 1);
        const auto code_limit = std::uint32_t{1} << bit_length;
        if (next_code + spec.code_counts[i] > code_limit) {
            return std::nullopt;
        }
        decoder.first_codes_[i] = next_code;
        decoder.first_symbols_[i] = symbol_index;
        decoder.code_counts_[i] = spec.code_counts[i];
        symbol_index += spec.code_counts[i];
        next_code = (next_code + spec.code_counts[i]) << 1U;
    }
    return decoder;
}

} // namespace echo::image
