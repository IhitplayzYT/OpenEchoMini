#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

// Partial model of module 80's range-normalize and cumulative-symbol helpers.
// The caller supplies the firmware's adaptive cumulative table for each symbol.
class Module80ApeRangeDecoder {
public:
    Module80ApeRangeDecoder(std::uint32_t range,
                            std::uint32_t code,
                            std::span<const std::uint8_t> refill_bytes) noexcept;

    [[nodiscard]] std::optional<std::size_t> decode_symbol(
        std::span<const std::uint16_t> cumulative,
        std::span<const std::uint16_t> range_scale) noexcept;
    [[nodiscard]] std::uint32_t range() const noexcept;
    [[nodiscard]] std::uint32_t code() const noexcept;
    [[nodiscard]] std::size_t bytes_consumed() const noexcept;
    [[nodiscard]] bool failed() const noexcept;

private:
    [[nodiscard]] bool normalize() noexcept;

    std::uint32_t range_;
    std::uint32_t code_;
    std::span<const std::uint8_t> refill_bytes_;
    std::size_t cursor_{};
    bool failed_{};
};

} // namespace echo::audio
