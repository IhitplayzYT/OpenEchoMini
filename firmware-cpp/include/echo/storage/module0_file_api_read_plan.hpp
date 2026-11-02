#pragma once

#include "echo/storage/module0_file_api_dispatch.hpp"
#include "echo/storage/module0_stream_write_plan.hpp"

#include <algorithm>
#include <cstdint>
#include <span>

namespace echo::storage {

struct Module0FileApiReadPlan {
    bool handle_open{};
    std::uint32_t starting_position{};
    std::uint32_t file_length{};
    std::uint32_t requested_bytes{};
    std::uint32_t bytes_available{};
    std::uint32_t bytes_to_read{};
    std::uint32_t ending_position{};
    std::uint32_t first_cluster{};
    std::uint32_t current_cluster{};

    [[nodiscard]] constexpr bool reaches_eof() const noexcept {
        return bytes_to_read < requested_bytes;
    }
};

// Models FUN_03059d8e's handle/EOF bounds. Unlike writes, reads may begin at
// an unaligned byte position; the firmware reads aligned 512-byte sectors
// into a temporary buffer and copies the requested byte range. Device errors
// may shorten the actual result and are outside this successful-read plan.
[[nodiscard]] inline Module0FileApiReadPlan plan_module0_file_api_read(
    const std::span<const std::uint8_t, kModule0ResourceHandleSlotBytes> slot,
    const std::uint32_t requested_bytes) noexcept {
    Module0FileApiReadPlan plan{};
    plan.handle_open = slot[0] != 0U;
    plan.starting_position = module0_file_api_tell(slot);
    plan.file_length = module0_file_api_length(slot);
    plan.requested_bytes = requested_bytes;
    plan.first_cluster = module0_file_api_read_u32_le(
        slot, kModule0HandleFirstClusterOffset);
    plan.current_cluster = module0_file_api_read_u32_le(
        slot, kModule0HandleCurrentClusterOffset);

    if (!plan.handle_open || plan.starting_position >= plan.file_length) {
        plan.ending_position = plan.starting_position;
        return plan;
    }

    plan.bytes_available = plan.file_length - plan.starting_position;
    plan.bytes_to_read = std::min(requested_bytes, plan.bytes_available);
    plan.ending_position = plan.starting_position + plan.bytes_to_read;
    return plan;
}

} // namespace echo::storage
