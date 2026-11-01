#include "echo/audio/module73_subband_profile.hpp"

namespace echo::audio {

namespace {

constexpr std::array<Module73SubbandTable, 5> kSubbandTables{{
    {27, {7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 3, 3, 3, 3, 3,
          3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {30, {7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 3, 3, 3, 3, 3,
          3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {8,  {5, 5, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0,
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {12, {5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0,
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {30, {4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0}},
}};

} // namespace

std::span<const Module73SubbandTable, 5>
module73_subband_profile_tables() noexcept {
    return kSubbandTables;
}

Module73SubbandProfile select_module73_subband_profile(
    const Module73SubbandProfileInput& input) noexcept {
    const bool mode2 = input.channel_mode == 2;
    const std::uint32_t channels = input.channel_mode == 0 ? 1U : 2U;
    const std::uint32_t effective_bitrate =
        channels == 2 ? input.bitrate >> 1U : input.bitrate;

    std::uint8_t table_index = 0;
    if ((input.stream_flags & 0x1000U) != 0) {
        table_index = 4;
    } else if ((input.stream_flags & 0x0400U) != 0) {
        table_index = input.sample_rate == 48000U ? 0 : 1;
    } else if (effective_bitrate < 48001U) {
        table_index = input.sample_rate == 32000U ? 3 : 2;
    } else if (effective_bitrate > 80000U) {
        table_index = input.sample_rate == 48000U ? 0 : 1;
    }

    const std::uint32_t mode_limit =
        mode2 ? input.mode2_band_limit * 4U + 4U : 32U;
    const std::uint8_t table_band_count = kSubbandTables[table_index].band_count;
    return {table_index, static_cast<std::uint8_t>(channels), effective_bitrate,
            mode_limit,
            static_cast<std::uint8_t>(mode_limit < table_band_count
                ? mode_limit
                : table_band_count),
            mode2};
}

} // namespace echo::audio
