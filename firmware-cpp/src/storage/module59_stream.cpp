#include "echo/storage/module59_stream.hpp"

#include "echo/storage/module59_record_transform.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace echo::storage {
namespace {

Module59StreamReadPlan failure(const Module59StreamPlanStatus status,
                               const std::int32_t firmware_result) {
    Module59StreamReadPlan plan{};
    plan.status = status;
    plan.firmware_result = firmware_result;
    return plan;
}

} // namespace

Module59StreamReadPlan plan_module59_stream_read(
    const Module59StreamContext* context,
    const std::int32_t requested_length,
    const bool destination_available) noexcept {
    if (requested_length <= 0 || !destination_available) {
        return failure(Module59StreamPlanStatus::no_op, 0);
    }
    if (context == nullptr) {
        return failure(Module59StreamPlanStatus::invalid_context, -2);
    }

    Module59StreamBackend backend{};
    switch (context->mode) {
    case Module59StreamMode::closed:
        return failure(Module59StreamPlanStatus::closed, -3);
    case Module59StreamMode::file_read:
    case Module59StreamMode::file_read_write:
        backend = Module59StreamBackend::file;
        break;
    case Module59StreamMode::memory_primary:
    case Module59StreamMode::memory_secondary:
        backend = Module59StreamBackend::memory;
        break;
    default:
        return failure(Module59StreamPlanStatus::invalid_mode, -4);
    }
    if (context->cursor < 0) {
        return failure(Module59StreamPlanStatus::negative_cursor, -5);
    }

    const std::int64_t end = context->configured_length == 0
        ? context->end_if_unsized
        : static_cast<std::int64_t>(context->logical_base) +
              context->configured_length;
    if (end < std::numeric_limits<std::int32_t>::min() ||
        end > std::numeric_limits<std::int32_t>::max()) {
        return failure(Module59StreamPlanStatus::invalid_bounds, -5);
    }
    if (end <= context->cursor) {
        return failure(Module59StreamPlanStatus::at_end, -1);
    }

    const auto available = end - context->cursor;
    const auto count = std::min<std::int64_t>(requested_length, available);
    const std::int64_t source = backend == Module59StreamBackend::memory
        ? static_cast<std::int64_t>(context->memory_base_address) + context->cursor
        : context->cursor;
    const std::int64_t transform = context->logical_base < 0
        ? static_cast<std::int64_t>(context->cursor) - context->logical_base
        : context->cursor;
    const std::int64_t next = static_cast<std::int64_t>(context->cursor) + count;
    constexpr auto max_u32 = std::numeric_limits<std::uint32_t>::max();
    if (source < 0 || source > max_u32 || transform < 0 ||
        transform > max_u32 || next > std::numeric_limits<std::int32_t>::max()) {
        return failure(Module59StreamPlanStatus::invalid_bounds, -5);
    }

    Module59StreamReadPlan plan{};
    plan.status = Module59StreamPlanStatus::read;
    plan.backend = backend;
    plan.firmware_result = static_cast<std::int32_t>(count);
    plan.cursor_before = context->cursor;
    plan.next_cursor = static_cast<std::int32_t>(next);
    plan.read_length = static_cast<std::int32_t>(count);
    plan.source_offset = static_cast<std::uint32_t>(source);
    plan.transform_offset = static_cast<std::uint32_t>(transform);
    plan.transform_total_length = static_cast<std::uint32_t>(context->end_if_unsized);
    plan.transform_key_offset = context->transform_key_offset;
    return plan;
}

Module59StreamSeekPlan plan_module59_stream_seek(
    const Module59StreamContext* context,
    const Module59SeekOrigin origin,
    const std::int32_t displacement) noexcept {
    if (context == nullptr) {
        return {Module59StreamPlanStatus::invalid_context,
                Module59StreamBackend::file, -2, 0, false};
    }

    Module59StreamBackend backend{};
    switch (context->mode) {
    case Module59StreamMode::closed:
        return {Module59StreamPlanStatus::closed,
                Module59StreamBackend::file, -3, 0, false};
    case Module59StreamMode::file_read:
    case Module59StreamMode::file_read_write:
        backend = Module59StreamBackend::file;
        break;
    case Module59StreamMode::memory_primary:
    case Module59StreamMode::memory_secondary:
        backend = Module59StreamBackend::memory;
        break;
    default:
        return {Module59StreamPlanStatus::invalid_mode,
                Module59StreamBackend::file, -4, 0, false};
    }

    const std::int64_t end = context->configured_length == 0
        ? context->end_if_unsized
        : static_cast<std::int64_t>(context->logical_base) +
              context->configured_length;
    if (end < std::numeric_limits<std::int32_t>::min() ||
        end > std::numeric_limits<std::int32_t>::max() ||
        end < context->logical_base) {
        return {Module59StreamPlanStatus::invalid_bounds, backend, -5, 0, false};
    }

    std::int64_t start = 0;
    switch (origin) {
    case Module59SeekOrigin::begin:
        start = context->logical_base;
        break;
    case Module59SeekOrigin::current:
        start = context->cursor;
        break;
    case Module59SeekOrigin::end:
        start = end;
        break;
    default:
        return {Module59StreamPlanStatus::invalid_mode, backend, -4, 0, false};
    }
    auto target = start + displacement;
    if (target < context->logical_base) target = context->logical_base;
    if (target > end) target = end;
    if (target < std::numeric_limits<std::int32_t>::min() ||
        target > std::numeric_limits<std::int32_t>::max()) {
        return {Module59StreamPlanStatus::invalid_bounds, backend, -5, 0, false};
    }
    return {Module59StreamPlanStatus::seek, backend, 0,
            static_cast<std::int32_t>(target),
            backend == Module59StreamBackend::file};
}

bool apply_module59_stream_transforms(
    const std::span<std::uint8_t> bytes,
    const Module59StreamReadPlan& plan,
    const Module59TransformMap& transform_map) noexcept {
    if (plan.status != Module59StreamPlanStatus::read || plan.read_length < 0 ||
        static_cast<std::size_t>(plan.read_length) > bytes.size()) {
        return false;
    }

    std::size_t consumed = 0;
    std::uint64_t offset = plan.transform_offset;
    const auto length = static_cast<std::size_t>(plan.read_length);
    while (consumed < length) {
        if (offset > std::numeric_limits<std::uint32_t>::max()) return false;
        const auto range = transform_map.lookup(
            static_cast<std::uint32_t>(offset), plan.transform_total_length);
        if (!range) return false;
        const std::uint64_t range_end =
            static_cast<std::uint64_t>(range->start) + range->length;
        if (range_end <= offset) return false;
        const auto available = std::min<std::uint64_t>(
            length - consumed, range_end - offset);
        const auto segment_length = static_cast<std::size_t>(std::min<std::uint64_t>(
            available, static_cast<std::uint64_t>(
                           std::numeric_limits<std::int32_t>::max())));
        if (segment_length == 0) return false;

        std::array<std::uint8_t, 16> record_bytes{};
        for (std::size_t word = 0; word < range->record.size(); ++word) {
            const auto value = range->record[word];
            for (std::size_t byte = 0; byte < 4; ++byte) {
                record_bytes[word * 4 + byte] =
                    static_cast<std::uint8_t>(value >> (byte * 8));
            }
        }
        (void)transform_module59_record(
            bytes.subspan(consumed, segment_length),
            static_cast<std::int32_t>(segment_length), record_bytes,
            static_cast<std::uint32_t>(offset), plan.transform_key_offset);
        consumed += segment_length;
        offset += segment_length;
    }
    return true;
}

} // namespace echo::storage
