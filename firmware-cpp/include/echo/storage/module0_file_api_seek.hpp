#pragma once

#include "echo/storage/module0_file_api_dispatch.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule0HandleSeekAnchorClusterOffset = 0x24;
inline constexpr std::size_t kModule0HandleSeekAnchorPositionOffset = 0x28;

enum class Module0SeekPlanStatus {
    ready,
    inactive_handle,
    unsupported_mode,
};

struct Module0FileApiSeekPlan {
    Module0SeekPlanStatus status{Module0SeekPlanStatus::inactive_handle};
    std::uint8_t mode{};
    std::int32_t requested_offset{};
    std::uint32_t original_position{};
    std::uint32_t file_length{};
    std::uint32_t target_position{};
    std::uint32_t anchor_cluster{};
    std::uint32_t anchor_position{};
    bool resets_to_anchor{};

    [[nodiscard]] constexpr bool is_ready() const noexcept {
        return status == Module0SeekPlanStatus::ready;
    }
};

// Models the four position calculations in FUN_0305a286 and the shared
// clamp to [0, file_length]. Mode 3 also selects the slot's anchor cluster
// when the preliminary position reaches its anchor-position field. The
// firmware then resolves a FAT-chain or contiguous-cluster location; this
// planner returns that metadata hint but does not walk storage clusters.
[[nodiscard]] inline Module0FileApiSeekPlan plan_module0_file_api_seek(
    const std::span<const std::uint8_t, kModule0ResourceHandleSlotBytes> slot,
    const std::int32_t offset,
    const std::uint8_t mode) noexcept {
    Module0FileApiSeekPlan plan{};
    plan.mode = mode;
    plan.requested_offset = offset;
    plan.original_position = module0_file_api_tell(slot);
    plan.file_length = module0_file_api_length(slot);

    if (slot[0] == 0U) {
        plan.status = Module0SeekPlanStatus::inactive_handle;
        return plan;
    }
    if (mode > 3U) {
        plan.status = Module0SeekPlanStatus::unsupported_mode;
        return plan;
    }

    const auto offset_bits = static_cast<std::uint32_t>(offset);
    std::uint32_t target = 0;
    bool clamp_to_zero_when_past_end = offset < 1;
    switch (mode) {
    case 0:
        target = offset_bits;
        break;
    case 1:
        target = plan.original_position + offset_bits;
        break;
    case 2:
        target = plan.file_length - offset_bits;
        // The Thumb code negates the signed 32-bit offset before its shared
        // less-than-one check; preserve the observed 32-bit wrap behavior.
        {
            const std::uint32_t negated_offset = 0U - offset_bits;
            clamp_to_zero_when_past_end =
                (negated_offset & 0x80000000U) != 0U
                || negated_offset == 0U;
        }
        break;
    case 3:
        target = plan.original_position + offset_bits;
        plan.anchor_cluster = module0_file_api_read_u32_le(
            slot, kModule0HandleSeekAnchorClusterOffset);
        plan.anchor_position = module0_file_api_read_u32_le(
            slot, kModule0HandleSeekAnchorPositionOffset);
        plan.resets_to_anchor = target >= plan.anchor_position;
        break;
    default:
        break;
    }

    if (target > plan.file_length) {
        target = clamp_to_zero_when_past_end ? 0U : plan.file_length;
    }
    plan.target_position = target;
    plan.status = Module0SeekPlanStatus::ready;
    return plan;
}

} // namespace echo::storage
