#include "echo/audio/module73_layer3_band_tables.hpp"

#include <array>

namespace echo::audio {

namespace {

#include "module73_layer3_band_tables.inc"

constexpr std::span<const std::uint8_t> as_span(const auto& table) noexcept {
    return {table.data(), table.size()};
}

struct BandProfile {
    std::array<Module73Layer3BandWidthTable, 3> shapes;
};

const std::array<BandProfile, 9> kProfiles{{
    {{{{0x010249dcU, as_span(band_10249dc)},
       {0x01024a1eU, as_span(band_1024a1e)},
       {0x01024a93U, as_span(band_1024a93)}}}},
    {{{{0x010249f2U, as_span(band_10249f2)},
       {0x01024a45U, as_span(band_1024a45)},
       {0x01024ab9U, as_span(band_1024ab9)}}}},
    {{{{0x01024a08U, as_span(band_1024a08)},
       {0x01024a6cU, as_span(band_1024a6c)},
       {0x01024adfU, as_span(band_1024adf)}}}},
    {{{{0x01024b05U, as_span(band_1024b05)},
       {0x01024b31U, as_span(band_1024b31)},
       {0x01024ba6U, as_span(band_1024ba6)}}}},
    {{{{0x01024b1bU, as_span(band_1024b1b)},
       {0x01024b58U, as_span(band_1024b58)},
       {0x01024bcaU, as_span(band_1024bca)}}}},
    {{{{0x01024b1bU, as_span(band_1024b1b)},
       {0x01024b7fU, as_span(band_1024b7f)},
       {0x01024beeU, as_span(band_1024bee)}}}},
    {{{{0x01024b1bU, as_span(band_1024b1b)},
       {0x01024b7fU, as_span(band_1024b7f)},
       {0x01024beeU, as_span(band_1024bee)}}}},
    {{{{0x01024b1bU, as_span(band_1024b1b)},
       {0x01024b7fU, as_span(band_1024b7f)},
       {0x01024beeU, as_span(band_1024bee)}}}},
    {{{{0x01024c12U, as_span(band_1024c12)},
       {0x01024c28U, as_span(band_1024c28)},
       {0x01024c4fU, as_span(band_1024c4f)}}}},
}};

std::optional<unsigned> shape_index(const Module73Layer3BlockShape shape) noexcept {
    switch (shape) {
    case Module73Layer3BlockShape::long_block: return 0;
    case Module73Layer3BlockShape::short_block: return 1;
    case Module73Layer3BlockShape::mixed_block: return 2;
    }
    return std::nullopt;
}

} // namespace

std::optional<unsigned> module73_layer3_frequency_profile_slot(
    std::uint32_t packed_rate_word,
    const bool alternate_family) noexcept {
    if (alternate_family) packed_rate_word <<= 1U;
    const std::int32_t extracted =
        static_cast<std::int32_t>((packed_rate_word & 0x7ffU) >> 7U);
    const std::int32_t sign_correction =
        (packed_rate_word & 0x8000U) != 0 ? 1 : 0;
    const std::int32_t slot = extracted + sign_correction -
        (alternate_family ? 5 : 8);
    if (slot < 0 || slot >= 9) return std::nullopt;
    return static_cast<unsigned>(slot);
}

std::optional<Module73Layer3BandWidthTable> module73_layer3_band_widths(
    const unsigned frequency_profile_slot,
    const Module73Layer3BlockShape shape) noexcept {
    if (frequency_profile_slot >= kProfiles.size()) return std::nullopt;
    const auto index = shape_index(shape);
    if (!index) return std::nullopt;
    return kProfiles[frequency_profile_slot].shapes[*index];
}

} // namespace echo::audio
