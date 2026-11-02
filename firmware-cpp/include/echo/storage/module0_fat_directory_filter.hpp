#pragma once

#include <cstdint>
#include <span>

namespace echo::storage {

// Applies only to FUN_0305a6de's FAT-like directory-entry branch. Name bytes
// are the raw 11-byte FAT name field; no case conversion is performed here.
[[nodiscard]] bool module0_fat_directory_candidate_accepted(
    std::span<const std::uint8_t> name_field, std::uint8_t attributes,
    std::uint8_t backend_selector, std::uint8_t scan_mode) noexcept;

} // namespace echo::storage
