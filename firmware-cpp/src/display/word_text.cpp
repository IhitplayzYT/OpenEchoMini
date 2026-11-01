#include "echo/display/word_text.hpp"

#include <algorithm>

namespace echo::display {

std::size_t copy_or_swap_words(std::span<std::uint16_t> destination,
                               std::span<const std::uint16_t> source,
                               std::size_t count,
                               bool preserve_order) noexcept {
    const auto copied = std::min({count, destination.size(), source.size()});
    for (std::size_t i = 0; i < copied; ++i) {
        const auto value = source[i];
        destination[i] = preserve_order
                             ? value
                             : static_cast<std::uint16_t>((value << 8) |
                                                          (value >> 8));
    }
    return copied;
}

} // namespace echo::display
