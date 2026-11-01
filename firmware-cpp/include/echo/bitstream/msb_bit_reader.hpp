#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::bitstream {

// Bounded MSB-first reads matching the two-word extraction primitive in
// module 16. Input bytes are consumed in stream order; codec interpretation
// and refill policy remain the caller's responsibility.
class MsbBitReader {
public:
    explicit MsbBitReader(std::span<const std::uint8_t> bytes,
                          std::size_t initial_bit_offset = 0) noexcept;

    [[nodiscard]] std::optional<std::uint32_t> peek(unsigned count) const noexcept;
    [[nodiscard]] std::optional<std::uint32_t> read(unsigned count) noexcept;
    [[nodiscard]] std::size_t bits_consumed() const noexcept;
    [[nodiscard]] std::size_t bits_remaining() const noexcept;
    [[nodiscard]] std::size_t absolute_bit_position() const noexcept;

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t bit_origin_{};
    std::size_t bit_offset_{};
    bool valid_origin_{true};
};

} // namespace echo::bitstream
