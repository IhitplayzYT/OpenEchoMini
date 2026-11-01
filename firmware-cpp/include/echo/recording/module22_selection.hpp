#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::recording {

struct Module22SelectionGeometry {
    std::uint16_t normalized_selection{};
    std::uint8_t range_seed{};
    std::uint8_t range_origin{};
    std::uint8_t offset_in_range{};
    std::uint8_t visible_count{};
    std::uint8_t capacity_value{};
};

// Mirrors FUN_0307b53e/FUN_0307b564's selection-code and range arithmetic.
[[nodiscard]] std::uint16_t normalize_module22_selection(
    std::uint16_t code) noexcept;
[[nodiscard]] Module22SelectionGeometry compute_module22_selection_geometry(
    std::uint16_t code) noexcept;

enum class Module22SelectionEffect : std::uint8_t {
    notify_event,
    render_list,
    call_helper,
};

struct Module22SelectionAction {
    Module22SelectionEffect effect{};
    std::uint32_t target{};
};

struct Module22SelectionState {
    std::uint8_t current{};
    std::uint8_t previous{};
    std::uint8_t total{};
    std::uint8_t page_start{};
};

struct Module22SelectionPlan {
    Module22SelectionState state{};
    bool accepted{};
    bool changed{};
    std::vector<Module22SelectionAction> actions;
};

// Models FUN_0307bf5e's bounds check, selection/page state update, redraw,
// and event notifications. It does not interpret records or open files.
[[nodiscard]] Module22SelectionPlan plan_module22_selection_change(
    Module22SelectionState state, std::uint32_t target);

struct Module22ListRecord {
    std::uint16_t label_code{};
    std::uint16_t resource_code{};
    std::uint16_t opaque_word{};
};

[[nodiscard]] std::span<const Module22ListRecord>
module22_firmware_records() noexcept;

struct Module22VisibleRow {
    std::uint8_t row{};
    std::uint16_t item_index{};
    bool selected{};
    std::optional<std::uint16_t> selected_indicator_icon;
    bool record_available{};
    std::uint16_t rendered_label_code{};
    std::uint16_t resource_code{};
};

// Mirrors the renderer's six-byte record stride and six-row page loop. The
// third record word is retained but its use is unknown.
[[nodiscard]] std::vector<Module22VisibleRow> plan_module22_visible_rows(
    std::span<const Module22ListRecord> records, std::uint8_t total,
    std::uint8_t page_start, std::uint8_t selected_row);

} // namespace echo::recording
