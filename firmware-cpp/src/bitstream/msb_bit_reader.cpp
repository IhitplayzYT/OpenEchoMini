#include "echo/bitstream/msb_bit_reader.hpp"

#include <limits>

namespace echo::bitstream {

MsbBitReader::MsbBitReader(const std::span<const std::uint8_t> bytes,
                           const std::size_t initial_bit_offset) noexcept
    : bytes_(bytes), bit_origin_(initial_bit_offset), bit_offset_(initial_bit_offset) {
    constexpr auto max_bytes = std::numeric_limits<std::size_t>::max() / 8U;
    const auto total_bits = bytes.size() > max_bytes
                                ? std::numeric_limits<std::size_t>::max()
                                : bytes.size() * 8U;
    valid_origin_ = initial_bit_offset <= total_bits;
}

std::optional<std::uint32_t> MsbBitReader::peek(const unsigned count) const noexcept {
    if (!valid_origin_ || count > 32 || count > bits_remaining()) {
        return std::nullopt;
    }

    std::uint32_t value = 0;
    for (unsigned i = 0; i < count; ++i) {
        const auto absolute_bit = bit_offset_ + i;
        const auto byte = bytes_[absolute_bit / 8];
        const auto shift = 7U - static_cast<unsigned>(absolute_bit % 8);
        value = static_cast<std::uint32_t>((value << 1U) | ((byte >> shift) & 1U));
    }
    return value;
}

std::optional<std::uint32_t> MsbBitReader::read(const unsigned count) noexcept {
    const auto value = peek(count);
    if (!value) return std::nullopt;
    bit_offset_ += count;
    return value;
}

std::size_t MsbBitReader::bits_consumed() const noexcept {
    return bit_offset_ - bit_origin_;
}

std::size_t MsbBitReader::bits_remaining() const noexcept {
    if (!valid_origin_) return 0;
    constexpr auto max_bytes = std::numeric_limits<std::size_t>::max() / 8U;
    if (bytes_.size() > max_bytes) {
        return std::numeric_limits<std::size_t>::max() - bit_offset_;
    }
    const auto total_bits = bytes_.size() * 8U;
    return bit_offset_ > total_bits ? 0 : total_bits - bit_offset_;
}

std::size_t MsbBitReader::absolute_bit_position() const noexcept {
    return bit_offset_;
}

} // namespace echo::bitstream
