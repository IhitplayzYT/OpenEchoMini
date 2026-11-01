#include "echo/storage/module23_name_hash.hpp"

#include <algorithm>

namespace echo::storage {
namespace {

constexpr char16_t fold_ascii_lowercase(const char16_t unit) noexcept {
    return unit >= u'a' && unit <= u'z'
        ? static_cast<char16_t>(unit - (u'a' - u'A'))
        : unit;
}

constexpr char16_t unit_or_nul(const std::u16string_view value,
                               const std::size_t index) noexcept {
    return index < value.size() ? value[index] : u'\0';
}

} // namespace

std::uint32_t hash_module23_name(const std::u16string_view name) noexcept {
    auto hash = kModule23FnvOffsetBasis;
    for (const auto unit : name) {
        if (unit == u'\0') break;
        hash = (static_cast<std::uint32_t>(fold_ascii_lowercase(unit)) ^ hash) *
            kModule23FnvPrime;
    }
    return hash;
}

std::uint16_t hash_module23_catalog_name16(
    const std::u16string_view name) noexcept {
    std::uint32_t hash = 0;
    for (const auto unit : name) {
        if (unit == u'\0') break;
        hash = (static_cast<std::uint32_t>(fold_ascii_lowercase(unit)) +
                hash * 0x1fu) & 0xffffu;
    }
    return static_cast<std::uint16_t>(hash);
}

bool equal_module23_names_case_insensitive(const std::u16string_view left,
                                           const std::u16string_view right) noexcept {
    const auto count = std::max(left.size(), right.size()) + 1;
    for (std::size_t i = 0; i < count; ++i) {
        const auto left_unit = fold_ascii_lowercase(unit_or_nul(left, i));
        const auto right_unit = fold_ascii_lowercase(unit_or_nul(right, i));
        if (left_unit != right_unit) return false;
        if (left_unit == u'\0') return true;
    }
    return true;
}

Module23NameCacheResult probe_module23_name_cache_entry(
    const bool directory_matches, const std::uint32_t cached_hash,
    const std::uint32_t requested_hash, const bool entry_valid,
    const std::u16string_view cached_name,
    const std::u16string_view requested_name) noexcept {
    if (!directory_matches || cached_hash != requested_hash) {
        return Module23NameCacheResult::miss;
    }
    if (!entry_valid) return Module23NameCacheResult::matching_invalid_entry;
    return equal_module23_names_case_insensitive(cached_name, requested_name)
        ? Module23NameCacheResult::exact_match
        : Module23NameCacheResult::miss;
}

Module23NameCacheResult probe_module23_name_cache_entry(
    const Module23NameCacheEntry& entry,
    const std::uint32_t requested_directory,
    const std::uint32_t requested_hash,
    const std::u16string_view cached_name,
    const std::u16string_view requested_name) noexcept {
    return probe_module23_name_cache_entry(
        entry.directory_key == requested_directory,
        entry.fnv_hash, requested_hash, entry.valid, cached_name, requested_name);
}

} // namespace echo::storage
