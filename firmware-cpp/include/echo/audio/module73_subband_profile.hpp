#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace echo::audio {

struct Module73SubbandProfileInput {
    std::uint32_t stream_flags{};
    std::uint32_t bitrate{};
    std::uint32_t sample_rate{};
    std::uint8_t channel_mode{};
    std::uint32_t mode2_band_limit{};
};

struct Module73SubbandTable {
    std::uint8_t band_count{};
    std::array<std::uint8_t, 32> allocation_widths{};
};

struct Module73SubbandProfile {
    std::uint8_t table_index{};
    std::uint8_t channel_count{};
    std::uint32_t effective_bitrate{};
    std::uint32_t requested_subbands{};
    std::uint8_t active_subbands{};
    bool mode2_flag_set{};
};

[[nodiscard]] std::span<const Module73SubbandTable, 5>
module73_subband_profile_tables() noexcept;

// Models profile selection and active-band limits at FUN_010079d0. The rest
// of the decoder remains separate.
[[nodiscard]] Module73SubbandProfile select_module73_subband_profile(
    const Module73SubbandProfileInput& input) noexcept;

} // namespace echo::audio
