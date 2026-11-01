#include "echo/storage/module59_lifecycle.hpp"

#include <limits>

namespace echo::storage {

Module59ClosePlan plan_module59_close(
    const Module59StreamContext* context) noexcept {
    if (context == nullptr) {
        return {Module59LifecycleStatus::invalid_context, -2, false, false,
                false, false};
    }
    const auto mode = static_cast<std::uint8_t>(context->mode);
    if (mode == 0) {
        return {Module59LifecycleStatus::closed, -3, false, false, false,
                false};
    }
    if (mode > 4) {
        return {Module59LifecycleStatus::invalid_mode, -4, false, false,
                false, false};
    }

    const bool file_mode = mode == 1;
    const bool flush = file_mode && context->flush_or_grow_enabled;
    const bool close = file_mode && context->file_handle != -1;
    return {Module59LifecycleStatus::success, 0, flush, close, true, true};
}

Module59OpenPlan plan_module59_file_open(
    const Module59OpenRequest& request) noexcept {
    if (!request.context_available || !request.path_available) {
        const auto status = request.context_available
            ? Module59LifecycleStatus::invalid_path
            : Module59LifecycleStatus::invalid_context;
        return {status, -2, false, false, 0, -1, 0, 0,
                request.logical_base, false, 1, 1};
    }

    const bool cleanup_resets_context =
        request.initial_mode >= 1 && request.initial_mode <= 4;
    const auto base_after_cleanup = cleanup_resets_context
        ? 0
        : request.logical_base;
    const auto cursor_after_cleanup = cleanup_resets_context
        ? 0
        : request.initial_cursor;
    const auto size_after_cleanup = cleanup_resets_context
        ? 0
        : request.initial_file_size;
    const bool opened = request.opened_handle != -1;
    const auto cursor = base_after_cleanup < 0
        ? base_after_cleanup
        : (request.seek_to_base_result < 0
               ? base_after_cleanup
               : request.seek_to_base_result);
    return {opened ? Module59LifecycleStatus::success
                   : Module59LifecycleStatus::open_failed,
            opened ? 0 : -2,
            true,
            opened && base_after_cleanup >= 0,
            1,
            request.opened_handle,
            opened ? request.file_size : size_after_cleanup,
            opened ? cursor : cursor_after_cleanup,
            base_after_cleanup,
            false,
            1,
            1};
}

Module59LengthPlan plan_module59_configured_length(
    const Module59StreamContext* context,
    const Module59LengthRequest& request) noexcept {
    if (context == nullptr) {
        return {Module59LifecycleStatus::invalid_context, -2};
    }
    const auto mode = static_cast<std::uint8_t>(context->mode);
    if (mode == 0) {
        return {Module59LifecycleStatus::closed, -3};
    }
    if (mode > 4) {
        return {Module59LifecycleStatus::invalid_mode, -4};
    }

    const std::int64_t desired_end =
        static_cast<std::int64_t>(context->logical_base) +
        request.requested_length;
    if (desired_end < std::numeric_limits<std::int32_t>::min() ||
        desired_end > std::numeric_limits<std::int32_t>::max()) {
        return {Module59LifecycleStatus::invalid_bounds, -4};
    }

    Module59LengthPlan plan{};
    plan.resulting_file_size = context->end_if_unsized;
    plan.resulting_configured_length = context->configured_length;
    if (context->end_if_unsized >= desired_end) {
        plan.action = Module59LengthAction::set_requested;
        plan.resulting_configured_length = request.requested_length;
        return plan;
    }

    const bool file_mode = mode == 1 || mode == 2;
    if (!file_mode || !context->flush_or_grow_enabled) {
        const std::int64_t clipped_length =
            static_cast<std::int64_t>(context->end_if_unsized) -
            context->logical_base;
        if (clipped_length < std::numeric_limits<std::int32_t>::min() ||
            clipped_length > std::numeric_limits<std::int32_t>::max()) {
            return {Module59LifecycleStatus::invalid_bounds, -4};
        }
        plan.action = Module59LengthAction::clip_to_current_end;
        plan.resulting_configured_length =
            static_cast<std::int32_t>(clipped_length);
        return plan;
    }

    if (!request.backend_results_available) {
        plan.status = Module59LifecycleStatus::needs_backend_result;
        plan.action = Module59LengthAction::probe_growth;
        plan.asks_for_position_query = true;
        plan.seeks_to_requested_end = true;
        plan.restores_file_position = true;
        plan.seek_target = static_cast<std::int32_t>(desired_end);
        return plan;
    }

    plan.restores_file_position = true;
    plan.restore_position = request.file_position_before;
    if (request.seek_to_requested_end_result > context->end_if_unsized) {
        const std::int64_t new_length =
            static_cast<std::int64_t>(request.seek_to_requested_end_result) -
            context->logical_base;
        if (new_length < std::numeric_limits<std::int32_t>::min() ||
            new_length > std::numeric_limits<std::int32_t>::max()) {
            return {Module59LifecycleStatus::invalid_bounds, -4};
        }
        plan.action = Module59LengthAction::grew_file;
        plan.resulting_file_size = request.seek_to_requested_end_result;
        plan.resulting_configured_length = static_cast<std::int32_t>(new_length);
    } else {
        plan.action = Module59LengthAction::preserve_after_failed_growth;
    }
    return plan;
}

} // namespace echo::storage
