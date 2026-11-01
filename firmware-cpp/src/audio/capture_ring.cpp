#include "echo/audio/capture_ring.hpp"

namespace echo::audio {

std::size_t CaptureRing::append_words(std::span<const std::uint8_t> input,
                                      std::uint32_t mode) {
    const auto words = input.size() / 4;
    const auto bytes_per_word = mode == 0x0f ? std::size_t{4} : std::size_t{3};

    for (std::size_t word = 0; word < words; ++word) {
        const auto source = word * 4;
        const auto first_byte = mode == 0x0f ? std::size_t{0} : std::size_t{1};
        for (std::size_t byte = 0; byte < bytes_per_word; ++byte) {
            bytes_[cursor_] = input[source + first_byte + byte];
            cursor_ = (cursor_ + 1) % capacity;
        }
        total_written_ += bytes_per_word;
    }
    return words * bytes_per_word;
}

} // namespace echo::audio
