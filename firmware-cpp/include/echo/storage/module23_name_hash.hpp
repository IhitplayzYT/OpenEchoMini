#pragma once

#include <cstdint>
#include <array>
#include <cstddef>
#include <string_view>

namespace echo::storage {

inline constexpr std::uint32_t kModule23FnvOffsetBasis = 0x811c9dc5u;
inline constexpr std::uint32_t kModule23FnvPrime = 0x01000193u;
inline constexpr std::uint8_t kModule23NameCacheCapacity = 16;
inline constexpr std::uint16_t kModule23DirectoryIndexCapacity = 0x200;
inline constexpr std::size_t kModule23MetadataBytes = 0x2a;

// Host-side grouping of fields that firmware stores in separate BSS arrays.
struct Module23NameCacheEntry {
    std::uint32_t directory_key{};
    std::uint32_t fnv_hash{};
    bool valid{};
    std::array<std::uint8_t, kModule23MetadataBytes> metadata{};
    std::uint32_t record_id{};
    std::uint16_t item_index{};
    std::uint8_t attributes{};
};

// Hashes UTF-16 code units with the firmware's ASCII-only lowercase fold.
// The firmware stops at the first NUL and uses 32-bit wrapping arithmetic.
[[nodiscard]] std::uint32_t hash_module23_name(
    std::u16string_view name) noexcept;

// Models the directory-local 16-bit name index used by FUN_0307c34a.
[[nodiscard]] std::uint16_t hash_module23_catalog_name16(
    std::u16string_view name) noexcept;

// Compares complete NUL-terminated UTF-16 names with ASCII-only case folding.
[[nodiscard]] bool equal_module23_names_case_insensitive(
    std::u16string_view left, std::u16string_view right) noexcept;

enum class Module23NameCacheResult : std::uint8_t {
    miss = 0,
    exact_match = 1,
    matching_invalid_entry = 2,
};

[[nodiscard]] Module23NameCacheResult probe_module23_name_cache_entry(
    bool directory_matches, std::uint32_t cached_hash,
    std::uint32_t requested_hash, bool entry_valid,
    std::u16string_view cached_name,
    std::u16string_view requested_name) noexcept;

[[nodiscard]] Module23NameCacheResult probe_module23_name_cache_entry(
    const Module23NameCacheEntry& entry, std::uint32_t requested_directory,
    std::uint32_t requested_hash, std::u16string_view cached_name,
    std::u16string_view requested_name) noexcept;

} // namespace echo::storage
