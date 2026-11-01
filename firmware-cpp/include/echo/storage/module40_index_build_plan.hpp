#pragma once

#include <array>
#include <cstdint>

namespace echo::storage {

struct Module40PrimaryIndexPass {
    std::uint8_t worker_mode{};
    std::array<std::uint16_t, 4> worker_constants{};
    std::uint8_t constant_count{};
    std::array<std::uint32_t, 4> workspace_offsets{};
    std::uint8_t workspace_count{};
};

inline constexpr std::array<Module40PrimaryIndexPass, 4>
    module40_primary_index_passes{{
        {1, {0x400, 0, 0, 0}, 1, {0x52000, 0, 0, 0}, 1},
        {2, {0x14, 0x600, 0, 0}, 2, {0x52140, 0x52160, 0, 0}, 2},
        {3, {0x14, 0x600, 0x500, 0}, 3,
         {0x52020, 0x520c0, 0x52040, 0}, 3},
        {4, {0x14, 0x600, 0x500, 0x700}, 4,
         {0x521e0, 0x52300, 0x52280, 0x52200}, 4},
    }};

struct Module40IndexBuildControl {
    bool run_primary_passes{};
    std::uint16_t secondary_target{};
    std::uint16_t tertiary_target{};
    std::uint8_t repeated_worker_mode{2};
    std::uint16_t repeated_worker_scratch{0x300};
    std::uint16_t repeated_worker_limit{0x2c};
    std::uint32_t count_record_size{8};
};

[[nodiscard]] constexpr Module40IndexBuildControl plan_module40_index_build(
    const std::uint16_t primary_flag, const std::uint16_t secondary_count,
    const std::uint16_t tertiary_count) noexcept {
    return {primary_flag != 0, secondary_count, tertiary_count};
}

} // namespace echo::storage
