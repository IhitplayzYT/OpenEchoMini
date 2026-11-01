#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

// Safe model of the APE module's observed MSB-first bit extraction helper.
// It is a parsing primitive only; it does not implement APE frame decoding.
class ApeBitReader {
public:
    explicit ApeBitReader(std::span<const std::uint8_t> bytes) noexcept;
    [[nodiscard]] std::optional<std::uint8_t> read_bits(unsigned count) noexcept;
    [[nodiscard]] std::size_t bits_consumed() const noexcept;
    [[nodiscard]] std::size_t bits_remaining() const noexcept;

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t bit_offset_{};
};

} // namespace echo::audio
