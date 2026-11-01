#pragma once

#include <cstdint>
#include <array>
#include <optional>

namespace echo::storage {

// Values as consumed by the module 59 row walker. Their on-disk meaning is
// not established; names preserve the byte offsets used by the binary.
struct Module59RowContextFields {
    std::uint32_t record_count{};
    std::uint16_t field_0c{};
    std::uint16_t field_0e{};
    std::uint16_t row_stride_field_10{};
    std::uint16_t field_16{};
    std::uint32_t terminal_boundary_field_24{};
};

struct Module59RowRequest {
    std::uint32_t first_row{};
    std::uint32_t row_count{};
    std::uint32_t output_stride{};
    std::uint32_t output_address{};
};

struct Module59RowPlan {
    std::uint64_t first_byte_offset{};
    std::uint32_t row_stride{};
    std::uint32_t row_count{};
    std::uint32_t output_stride{};
    std::uint32_t output_address{};
};

struct Module59SingleRowReadPlan {
    std::uint32_t row_start{};
    std::uint32_t requested_end{};
    std::uint32_t effective_section_end{};
    std::array<std::uint32_t, 2> transform_prefetch_offsets{};
    std::uint8_t transform_prefetch_count{};
    bool uses_terminal_boundary{};
};

struct Module59RowDispatchPlan {
    std::uint32_t completed_rows{};
    bool clears_output_tail{};
    std::uint32_t clear_address{};
    std::uint32_t clear_length{};
};

// Mirrors FUN_0308dec6's start calculation and fixed-stride row stepping.
// Returns nullopt when observed address arithmetic exceeds the 32-bit domain.
[[nodiscard]] std::optional<Module59RowPlan> plan_module59_rows(
    const Module59RowContextFields& context,
    const Module59RowRequest& request) noexcept;

// Mirrors FUN_0308dc24's row-table bound and transform-boundary setup.
[[nodiscard]] std::optional<Module59SingleRowReadPlan>
plan_module59_single_row_read(const Module59RowContextFields& context,
                              std::uint32_t row_index) noexcept;

// Models FUN_0308db74's row-count selection and output-tail clearing. The
// firmware's u/v path preserves incoming r5 as the completed-row count.
[[nodiscard]] std::optional<Module59RowDispatchPlan>
plan_module59_row_dispatch(std::uint16_t header_type,
                           std::uint32_t requested_rows,
                           std::uint32_t output_stride,
                           std::uint32_t output_address,
                           std::uint32_t incoming_r5,
                           std::uint32_t walked_rows) noexcept;

} // namespace echo::storage
