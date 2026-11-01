#include "echo/audio/module73_layer3_exponent.hpp"

#include <array>

namespace echo::audio {

namespace {

constexpr std::array<std::uint8_t, 22> kPreflagAdjustments{
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 3, 3, 3, 2};

} // namespace

std::optional<std::uint8_t> module73_layer3_preflag_adjustment(
    const unsigned scalefactor_band) noexcept {
    if (scalefactor_band >= kPreflagAdjustments.size()) return std::nullopt;
    return kPreflagAdjustments[scalefactor_band];
}

std::optional<std::int32_t> module73_layer3_scale_exponent(
    const std::uint8_t global_gain,
    const std::uint8_t scalefactor,
    const bool scalefac_scale,
    const std::uint8_t preflag_adjustment,
    const std::uint8_t subblock_gain) noexcept {
    if (scalefactor > 31 || preflag_adjustment > 3 || subblock_gain > 7) {
        return std::nullopt;
    }
    const std::int32_t scale_shift = scalefac_scale ? 2 : 1;
    const std::int32_t adjusted_scalefactor =
        static_cast<std::int32_t>(scalefactor) + preflag_adjustment;
    return static_cast<std::int32_t>(global_gain) - 0xd2 -
        (adjusted_scalefactor << scale_shift) -
        static_cast<std::int32_t>(subblock_gain) * 8;
}

} // namespace echo::audio
