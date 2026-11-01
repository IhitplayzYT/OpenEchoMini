#pragma once

#include "echo/resources/offset_bank_directory.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::resources {

enum class FirmwareLocale : std::uint8_t {
    chinese_simplified,
    chinese_traditional,
    english,
    japanese,
    korean,
    french,
    german,
    italian,
    spanish,
    portuguese,
    russian,
    swedish,
    thai,
    polish,
    danish,
    dutch,
    greek,
    czech,
    turkish,
    hebrew,
    arabic,
};

// Returns the UTF-16LE text bytes for one observed fixed-slot string record.
// The byte range is relative to the complete f4 region.
[[nodiscard]] std::optional<ByteRange> localized_ui_text_range(
    std::span<const std::uint8_t> region,
    FirmwareLocale locale,
    std::size_t record_index) noexcept;

} // namespace echo::resources
