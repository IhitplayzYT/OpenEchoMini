#pragma once

#include <array>
#include <cstdint>

namespace echo::storage {

struct Module23ByteRead {
    std::uint32_t byte_offset{};
    std::uint32_t byte_count{};
};

struct Module23SortInfoInput {
    bool event_gate_matches{};
    bool state_gate_is_one{};
    std::uint16_t sort_mode{};
    std::uint32_t storage_base{};
    std::uint32_t context_word_34{};
    std::uint32_t context_word_38{};
    std::uint16_t first_read_index{};
    std::uint32_t descriptor_block_index{};
    std::array<std::uint32_t, 2> expected_words{};
    std::array<std::uint32_t, 2> observed_words{};
};

struct Module23SortInfoPlan {
    bool gates_passed{};
    bool setup_helper_called{};
    std::array<Module23ByteRead, 4> reads{};
    std::uint8_t read_count{};
    bool pair_matches{};
    std::uint8_t return_value{};
};

// Models FUN_0307b656's gating, byte-read offsets, and pair validation.
// It assumes caller-supplied read values and performs no storage I/O.
[[nodiscard]] Module23SortInfoPlan plan_module23_sort_info_check(
    const Module23SortInfoInput& input) noexcept;

struct Module23SortMenuGeometry {
    std::uint8_t first_item{};
    std::uint8_t visible_rows{};
    std::uint8_t selected_row{};
    std::uint8_t selected_item{};
};

// Mirrors FUN_0307d624's six-row/tail-three-row selection layout.
[[nodiscard]] Module23SortMenuGeometry compute_module23_sort_menu_geometry(
    std::uint16_t selection_code) noexcept;

} // namespace echo::storage
