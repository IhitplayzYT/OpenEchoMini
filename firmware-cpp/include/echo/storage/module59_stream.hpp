#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "echo/storage/module59_transform_map.hpp"

namespace echo::storage {

enum class Module59StreamMode : std::uint8_t {
    closed = 0,
    file_read = 1,
    file_read_write = 2,
    memory_primary = 3,
    memory_secondary = 4,
};

enum class Module59StreamBackend : std::uint8_t {
    file,
    memory,
};

enum class Module59StreamPlanStatus : std::uint8_t {
    read,
    seek,
    no_op,
    invalid_context,
    closed,
    invalid_mode,
    negative_cursor,
    at_end,
    invalid_bounds,
};

struct Module59StreamContext {
    Module59StreamMode mode{Module59StreamMode::closed};
    std::uint32_t transform_key_offset{};
    std::int32_t logical_base{};
    std::int32_t configured_length{};
    std::int32_t end_if_unsized{};
    std::int32_t cursor{};
    std::uint32_t memory_base_address{};
    std::int32_t file_handle{};
    bool flush_or_grow_enabled{};
};

struct Module59StreamReadPlan {
    Module59StreamPlanStatus status{Module59StreamPlanStatus::no_op};
    Module59StreamBackend backend{Module59StreamBackend::file};
    std::int32_t firmware_result{};
    std::int32_t cursor_before{};
    std::int32_t next_cursor{};
    std::int32_t read_length{};
    std::uint32_t source_offset{};
    std::uint32_t transform_offset{};
    std::uint32_t transform_total_length{};
    std::uint32_t transform_key_offset{};
};

enum class Module59SeekOrigin : std::uint8_t {
    begin = 0,
    current = 1,
    end = 2,
};

struct Module59StreamSeekPlan {
    Module59StreamPlanStatus status{Module59StreamPlanStatus::no_op};
    Module59StreamBackend backend{Module59StreamBackend::file};
    std::int32_t firmware_result{};
    std::int32_t target_cursor{};
    bool requires_physical_seek{};
};

// Mirrors the validation, end clamp, and address selection in FUN_0308cc18.
// It does not perform file I/O, copy memory, or apply record transforms.
[[nodiscard]] Module59StreamReadPlan plan_module59_stream_read(
    const Module59StreamContext* context,
    std::int32_t requested_length,
    bool destination_available) noexcept;

// Mirrors FUN_0308ca92's begin/current/end seek and logical-range clamp.
[[nodiscard]] Module59StreamSeekPlan plan_module59_stream_seek(
    const Module59StreamContext* context,
    Module59SeekOrigin origin,
    std::int32_t displacement) noexcept;

// Applies the module's transform record to each descriptor interval in a
// previously read buffer. Identity gaps pass through unchanged.
[[nodiscard]] bool apply_module59_stream_transforms(
    std::span<std::uint8_t> bytes,
    const Module59StreamReadPlan& plan,
    const Module59TransformMap& transform_map) noexcept;

} // namespace echo::storage
