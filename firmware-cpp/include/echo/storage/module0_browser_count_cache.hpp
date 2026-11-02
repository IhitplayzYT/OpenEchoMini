#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace echo::storage {

inline constexpr std::size_t kModule0BrowserCountCacheCapacity = 6;

enum class Module0BrowserCountKind : std::uint8_t {
    filtered_files,
    directories,
};

struct Module0BrowserCountCacheEntry {
    // Firmware compares these three values without assigning application
    // names to the first two.
    std::uint32_t key0{};
    std::uint32_t key1{};
    std::uint16_t mode{};
    std::uint16_t filtered_file_count{};
    std::uint16_t directory_count{};
    bool valid{};
};

struct Module0BrowserCountCache {
    std::array<Module0BrowserCountCacheEntry,
               kModule0BrowserCountCacheCapacity> entries{};
    std::uint8_t next_slot{};
};

// Models the six-entry lookup used by FUN_030549f2/FUN_03054b24.
[[nodiscard]] std::optional<std::uint16_t> module0_browser_count_cache_find(
    const Module0BrowserCountCache& cache, std::uint32_t key0,
    std::uint32_t key1, std::uint16_t mode,
    Module0BrowserCountKind kind) noexcept;

// Models FUN_03054c4e's unconditional round-robin write. The two result
// halfwords are separate in the firmware: filtered-file count at +0x3c and
// directory count at +0x48 in its structure-of-arrays cache block.
void module0_browser_count_cache_record(
    Module0BrowserCountCache& cache, std::uint32_t key0, std::uint32_t key1,
    std::uint16_t mode, std::uint16_t filtered_file_count,
    std::uint16_t directory_count) noexcept;

} // namespace echo::storage
