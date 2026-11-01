#pragma once

#include "echo/audio/module73_layer3_scalefactors.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct Module73Layer3BandWidthTable {
    std::uint32_t firmware_data_address{};
    std::span<const std::uint8_t> widths{};
};

// Mirrors FUN_01005a40's selector arithmetic for the packed context word at
// frame_context+0x10 and alternate-family flag at frame_context+0x18.
[[nodiscard]] std::optional<unsigned>
module73_layer3_frequency_profile_slot(
    std::uint32_t packed_rate_word,
    bool alternate_family) noexcept;

// Selects the observed long/short/mixed line-width table from the 9-slot
// pointer matrix rooted at 0x01024c78. The slot-to-header mapping remains
// caller-owned until the version/sample-rate selector is fully traced.
[[nodiscard]] std::optional<Module73Layer3BandWidthTable>
module73_layer3_band_widths(
    unsigned frequency_profile_slot,
    Module73Layer3BlockShape shape) noexcept;

} // namespace echo::audio
