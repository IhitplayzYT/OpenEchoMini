#include "echo/audio/vorbis_floor0.hpp"

#include <algorithm>
#include <limits>

namespace echo::audio {
namespace {

class LsbBitReader {
public:
    explicit LsbBitReader(std::span<const std::uint8_t> bytes) noexcept : bytes_(bytes) {}

    [[nodiscard]] bool read(std::uint8_t width, std::uint32_t& value) noexcept {
        if (width > 32 || bit_offset_ > bytes_.size() * 8 ||
            width > bytes_.size() * 8 - bit_offset_) {
            return false;
        }
        value = 0;
        for (std::uint8_t i = 0; i < width; ++i, ++bit_offset_) {
            const auto byte_index = bit_offset_ / 8;
            const auto bit_index = static_cast<unsigned>(bit_offset_ % 8);
            value |= static_cast<std::uint32_t>((bytes_[byte_index] >> bit_index) & 1U) << i;
        }
        return true;
    }

    [[nodiscard]] std::size_t bit_offset() const noexcept { return bit_offset_; }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t bit_offset_{};
};

bool read_lsb(std::span<const std::uint8_t> packet, std::size_t& bit_offset,
              std::uint8_t width, std::uint32_t& value) noexcept {
    if (width > 32 || bit_offset > packet.size() * 8 ||
        width > packet.size() * 8 - bit_offset) {
        return false;
    }
    value = 0;
    for (std::uint8_t i = 0; i < width; ++i, ++bit_offset) {
        const auto byte_index = bit_offset / 8;
        const auto bit_index = static_cast<unsigned>(bit_offset % 8);
        value |= static_cast<std::uint32_t>((packet[byte_index] >> bit_index) & 1U) << i;
    }
    return true;
}

std::uint8_t ilog(std::uint32_t value) noexcept {
    std::uint8_t width = 0;
    while (value != 0) {
        ++width;
        value >>= 1U;
    }
    return width;
}

} // namespace

std::optional<VorbisFloor0Setup> parse_vorbis_floor0_setup(
    std::span<const std::uint8_t> setup_bits,
    std::span<const std::uint16_t> codebook_dimensions) {
    LsbBitReader bits(setup_bits);
    std::uint32_t value = 0;
    VorbisFloor0Setup setup;
    if (!bits.read(8, value)) return std::nullopt;
    setup.order = static_cast<std::uint16_t>(value);
    if (!bits.read(16, value)) return std::nullopt;
    setup.rate = static_cast<std::uint16_t>(value);
    if (!bits.read(16, value)) return std::nullopt;
    setup.bark_map_size = static_cast<std::uint16_t>(value);
    if (!bits.read(6, value)) return std::nullopt;
    setup.amplitude_bits = static_cast<std::uint8_t>(value);
    if (!bits.read(8, value)) return std::nullopt;
    setup.amplitude_offset = static_cast<std::uint16_t>(value);
    if (setup.order == 0 || setup.rate == 0 || setup.bark_map_size == 0 ||
        !bits.read(4, value)) {
        return std::nullopt;
    }

    const auto book_count = value + 1U;
    setup.books.reserve(book_count);
    setup.book_dimensions.reserve(book_count);
    for (std::uint32_t i = 0; i < book_count; ++i) {
        if (!bits.read(8, value) || value >= codebook_dimensions.size() ||
            codebook_dimensions[value] == 0) {
            return std::nullopt;
        }
        setup.books.push_back(static_cast<std::uint8_t>(value));
        setup.book_dimensions.push_back(codebook_dimensions[value]);
    }
    setup.bits_consumed = bits.bit_offset();
    return setup;
}

std::optional<VorbisFloor0Packet> decode_vorbis_floor0_packet(
    std::span<const std::uint8_t> packet,
    const VorbisFloor0Setup& setup,
    const VorbisFloor0VectorDecode& decode_vector) {
    std::size_t bit_offset = 0;
    return decode_vorbis_floor0_packet(packet, bit_offset, setup, decode_vector);
}

std::optional<VorbisFloor0Packet> decode_vorbis_floor0_packet(
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    const VorbisFloor0Setup& setup,
    const VorbisFloor0VectorDecode& decode_vector) {
    if (setup.order == 0 || setup.amplitude_bits > 32 || setup.books.empty() ||
        setup.books.size() > 256 ||
        setup.books.size() != setup.book_dimensions.size() || !decode_vector) {
        return std::nullopt;
    }

    if (bit_offset > packet.size() * 8) return std::nullopt;
    const auto start_offset = bit_offset;
    std::uint32_t amplitude = 0;
    if (!read_lsb(packet, bit_offset, setup.amplitude_bits, amplitude)) return std::nullopt;

    VorbisFloor0Packet result;
    result.amplitude = amplitude;
    if (amplitude == 0) {
        result.present = false;
        result.bits_consumed = bit_offset - start_offset;
        return result;
    }

    result.present = true;
    const auto selector_width = ilog(static_cast<std::uint32_t>(setup.books.size()));
    std::uint32_t book_index = 0;
    if (!read_lsb(packet, bit_offset, selector_width, book_index) ||
        book_index >= setup.books.size()) {
        return std::nullopt;
    }
    result.book_index = static_cast<std::uint8_t>(book_index);

    std::vector<std::int32_t> coded_values;
    coded_values.reserve(setup.order);
    std::vector<std::size_t> vector_lengths;
    vector_lengths.reserve((setup.order + 1U) / 2U);
    std::size_t decoded_count = 0;
    while (decoded_count < setup.order) {
        const auto dimension = setup.book_dimensions[book_index];
        if (dimension == 0) return std::nullopt;
        const auto before = bit_offset;
        const auto vector = decode_vector(setup.books[book_index], dimension, packet, bit_offset);
        if (!vector || vector->size() != dimension || bit_offset < before ||
            bit_offset > packet.size() * 8) {
            return std::nullopt;
        }
        const auto take = std::min<std::size_t>(dimension, setup.order - decoded_count);
        coded_values.insert(coded_values.end(), vector->begin(), vector->begin() + take);
        vector_lengths.push_back(take);
        decoded_count += take;
    }

    result.lsp_values.reserve(setup.order);
    std::int64_t accumulator = 0;
    std::size_t value_index = 0;
    for (const auto vector_length : vector_lengths) {
        for (std::size_t i = 0; i < vector_length; ++i, ++value_index) {
            const auto unwrapped = accumulator + coded_values[value_index];
            if (unwrapped < std::numeric_limits<std::int32_t>::min() ||
                unwrapped > std::numeric_limits<std::int32_t>::max()) {
                return std::nullopt;
            }
            result.lsp_values.push_back(static_cast<std::int32_t>(unwrapped));
        }
        accumulator = result.lsp_values.back();
    }

    const auto amplitude_max = (std::uint64_t{1} << setup.amplitude_bits) - 1U;
    const auto gain = static_cast<std::uint64_t>(setup.amplitude_offset) *
        static_cast<std::uint64_t>(amplitude) * 16U / amplitude_max;
    if (gain > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    result.gain_numerator = static_cast<std::uint32_t>(gain);
    result.bits_consumed = bit_offset - start_offset;
    return result;
}

} // namespace echo::audio
