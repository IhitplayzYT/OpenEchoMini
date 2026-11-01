#pragma once

#include <cstdint>

#include "echo/storage/module59_stream.hpp"

namespace echo::storage {

enum class Module59LifecycleStatus : std::uint8_t {
    success,
    invalid_context,
    invalid_path,
    closed,
    invalid_mode,
    open_failed,
    needs_backend_result,
    invalid_bounds,
};

struct Module59ClosePlan {
    Module59LifecycleStatus status{Module59LifecycleStatus::success};
    std::int32_t firmware_result{};
    bool flush_file{};
    bool close_file{};
    bool clear_transform_ranges{};
    bool reset_context{};
};

struct Module59OpenRequest {
    bool context_available{true};
    bool path_available{true};
    std::uint8_t initial_mode{};
    std::int32_t opened_handle{-1};
    std::int32_t file_size{};
    std::int32_t initial_file_size{};
    std::int32_t initial_cursor{};
    // Values present before the close call at the start of the open path.
    // A valid nonzero stream mode is reset by close; state 0/invalid is not.
    std::int32_t logical_base{};
    std::int32_t seek_to_base_result{-1};
};

struct Module59OpenPlan {
    Module59LifecycleStatus status{Module59LifecycleStatus::success};
    std::int32_t firmware_result{};
    bool calls_close_helper{};
    bool requests_seek_to_base{};
    std::uint8_t resulting_mode{};
    std::int32_t resulting_handle{-1};
    std::int32_t resulting_file_size{};
    std::int32_t resulting_cursor{};
    std::int32_t resulting_logical_base{};
    bool resulting_flush_or_grow_enabled{};
    std::uint8_t open_flag_a{1};
    std::uint8_t open_flag_b{1};
};

enum class Module59LengthAction : std::uint8_t {
    set_requested,
    clip_to_current_end,
    probe_growth,
    grew_file,
    preserve_after_failed_growth,
};

struct Module59LengthRequest {
    std::int32_t requested_length{};
    bool backend_results_available{};
    std::int32_t file_position_before{};
    std::int32_t seek_to_requested_end_result{-1};
};

struct Module59LengthPlan {
    Module59LifecycleStatus status{Module59LifecycleStatus::success};
    std::int32_t firmware_result{};
    Module59LengthAction action{Module59LengthAction::set_requested};
    bool asks_for_position_query{};
    bool seeks_to_requested_end{};
    bool restores_file_position{};
    std::int32_t seek_target{};
    std::int32_t restore_position{};
    std::int32_t resulting_file_size{};
    std::int32_t resulting_configured_length{};
};

// Models the validation and resource actions observed in FUN_0308c9bc.
[[nodiscard]] Module59ClosePlan plan_module59_close(
    const Module59StreamContext* context) noexcept;

// Models the post-backend state produced by the file-open path. The caller
// supplies the already-observed open, size, and seek results.
[[nodiscard]] Module59OpenPlan plan_module59_file_open(
    const Module59OpenRequest& request) noexcept;

// Models FUN_0308cba2's requested-length clamp or optional file growth.
[[nodiscard]] Module59LengthPlan plan_module59_configured_length(
    const Module59StreamContext* context,
    const Module59LengthRequest& request) noexcept;

} // namespace echo::storage
