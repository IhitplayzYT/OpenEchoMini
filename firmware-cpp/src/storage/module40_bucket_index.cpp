#include "echo/storage/module40_bucket_index.hpp"

namespace echo::storage {

std::size_t select_module40_bucket(
    const std::span<const std::uint16_t> thresholds,
    const std::uint16_t normalized_key) noexcept {
    std::size_t index = 0;
    while (index < thresholds.size() && thresholds[index] < normalized_key) {
        ++index;
    }
    return index;
}

} // namespace echo::storage
