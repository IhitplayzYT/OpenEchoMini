#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::audio {

class CaptureRing {
public:
    static constexpr std::size_t capacity = 0xc000;

    // Appends complete 32-bit input words using the two packing branches
    // observed in HIFIEC40 module 63. Mode 0x0f retains all four bytes;
    // other observed modes copy bytes 1..3 from each word. A trailing partial
    // word is ignored, matching the firmware's input_bytes >> 2 loop bound.
    [[nodiscard]] std::size_t append_words(
        std::span<const std::uint8_t> input,
        std::uint32_t mode);

    [[nodiscard]] const std::array<std::uint8_t, capacity>& bytes() const noexcept {
        return bytes_;
    }
    [[nodiscard]] std::size_t cursor() const noexcept { return cursor_; }
    [[nodiscard]] std::uint64_t total_written() const noexcept { return total_written_; }

private:
    std::array<std::uint8_t, capacity> bytes_{};
    std::size_t cursor_{};
    std::uint64_t total_written_{};
};

} // namespace echo::audio
