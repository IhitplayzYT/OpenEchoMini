#include "echo/audio/ape_bit_reader.hpp"

namespace echo::audio {

ApeBitReader::ApeBitReader(std::span<const std::uint8_t> bytes) noexcept
    : bytes_(bytes) {}

std::optional<std::uint8_t> ApeBitReader::read_bits(unsigned count) noexcept {
    if (count > 8 || count > bits_remaining()) return std::nullopt;

    std::uint8_t value = 0;
    for (unsigned i = 0; i < count; ++i) {
        const auto absolute_bit = bit_offset_ + i;
        const auto byte = bytes_[absolute_bit / 8];
        const auto shift = 7u - static_cast<unsigned>(absolute_bit % 8);
        value = static_cast<std::uint8_t>((value << 1) | ((byte >> shift) & 1u));
    }
    bit_offset_ += count;
    return value;
}

std::size_t ApeBitReader::bits_consumed() const noexcept { return bit_offset_; }

std::size_t ApeBitReader::bits_remaining() const noexcept {
    return bytes_.size() * 8 - bit_offset_;
}

} // namespace echo::audio
