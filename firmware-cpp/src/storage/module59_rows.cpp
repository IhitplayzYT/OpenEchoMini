#include "echo/storage/module59_rows.hpp"

#include <limits>

namespace echo::storage {

std::optional<Module59RowPlan> plan_module59_rows(
    const Module59RowContextFields& context,
    const Module59RowRequest& request) noexcept {
    const std::uint64_t start =
        static_cast<std::uint64_t>(request.first_row) * context.row_stride_field_10 +
        static_cast<std::uint64_t>(context.field_0c) * context.field_0e +
        context.field_16;
    constexpr auto max_offset = std::numeric_limits<std::uint32_t>::max();
    if (start > max_offset) return std::nullopt;

    if (request.row_count != 0) {
        const std::uint64_t last_start = start +
            static_cast<std::uint64_t>(request.row_count - 1) *
                context.row_stride_field_10;
        if (last_start > max_offset) return std::nullopt;
    }

    return Module59RowPlan{start, context.row_stride_field_10,
                           request.row_count, request.output_stride,
                           request.output_address};
}

std::optional<Module59SingleRowReadPlan> plan_module59_single_row_read(
    const Module59RowContextFields& context,
    const std::uint32_t row_index) noexcept {
    const std::uint64_t row_start =
        static_cast<std::uint64_t>(row_index) * context.row_stride_field_10 +
        static_cast<std::uint64_t>(context.field_0c) * context.field_0e +
        context.field_16;
    const std::uint64_t requested_end = row_start + context.row_stride_field_10;
    const std::uint64_t table_end =
        static_cast<std::uint64_t>(context.record_count) *
            context.row_stride_field_10 +
        static_cast<std::uint64_t>(context.field_0c) * context.field_0e +
        context.field_16;
    constexpr auto max_offset = std::numeric_limits<std::uint32_t>::max();
    if (row_start > max_offset || requested_end > max_offset ||
        table_end > max_offset || requested_end > table_end) {
        return std::nullopt;
    }

    const bool terminal = requested_end == table_end;
    const auto section_end = terminal
        ? context.terminal_boundary_field_24
        : static_cast<std::uint32_t>(requested_end);
    if (terminal) {
        return Module59SingleRowReadPlan{
            static_cast<std::uint32_t>(row_start),
            static_cast<std::uint32_t>(requested_end), section_end,
            {static_cast<std::uint32_t>(row_start), 0}, 1, true};
    }
    return Module59SingleRowReadPlan{
        static_cast<std::uint32_t>(row_start),
        static_cast<std::uint32_t>(requested_end), section_end,
        {static_cast<std::uint32_t>(row_start),
         static_cast<std::uint32_t>(requested_end)}, 2, false};
}

std::optional<Module59RowDispatchPlan> plan_module59_row_dispatch(
    const std::uint16_t header_type,
    const std::uint32_t requested_rows,
    const std::uint32_t output_stride,
    const std::uint32_t output_address,
    const std::uint32_t incoming_r5,
    const std::uint32_t walked_rows) noexcept {
    const std::uint32_t completed =
        (header_type == 0x75 || header_type == 0x76) ? incoming_r5 : walked_rows;
    if (completed >= requested_rows) {
        return Module59RowDispatchPlan{completed, false, 0, 0};
    }

    const std::uint64_t clear_address =
        static_cast<std::uint64_t>(output_address) +
        static_cast<std::uint64_t>(output_stride) * completed;
    const std::uint64_t clear_length =
        static_cast<std::uint64_t>(output_stride) * (requested_rows - completed);
    constexpr auto max_address = std::numeric_limits<std::uint32_t>::max();
    if (clear_address > max_address || clear_length > max_address) {
        return std::nullopt;
    }
    return Module59RowDispatchPlan{completed, true,
                                   static_cast<std::uint32_t>(clear_address),
                                   static_cast<std::uint32_t>(clear_length)};
}

} // namespace echo::storage
