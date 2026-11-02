#include "echo/storage/module0_browser_count_cache.hpp"

namespace echo::storage {

std::optional<std::uint16_t> module0_browser_count_cache_find(
    const Module0BrowserCountCache& cache, const std::uint32_t key0,
    const std::uint32_t key1, const std::uint16_t mode,
    const Module0BrowserCountKind kind) noexcept {
    for (const auto& entry : cache.entries) {
        if (!entry.valid || entry.key0 != key0 || entry.key1 != key1 ||
            entry.mode != mode) {
            continue;
        }
        return kind == Module0BrowserCountKind::filtered_files
            ? entry.filtered_file_count
            : entry.directory_count;
    }
    return std::nullopt;
}

void module0_browser_count_cache_record(
    Module0BrowserCountCache& cache, const std::uint32_t key0,
    const std::uint32_t key1, const std::uint16_t mode,
    const std::uint16_t filtered_file_count,
    const std::uint16_t directory_count) noexcept {
    const auto slot = static_cast<std::size_t>(cache.next_slot) %
                      kModule0BrowserCountCacheCapacity;
    cache.entries[slot] = {key0, key1, mode, filtered_file_count,
                           directory_count, true};
    cache.next_slot = static_cast<std::uint8_t>(
        (slot + 1U) % kModule0BrowserCountCacheCapacity);
}

} // namespace echo::storage
