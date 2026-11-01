#include "echo/recording/module22_selection.hpp"

#include <algorithm>
#include <array>

namespace echo::recording {
namespace {

constexpr std::uint8_t divide_by_six(const std::uint8_t value) noexcept {
    // The firmware uses multiply-by-171 and a shift; /6 is equivalent for
    // the full byte range.
    return static_cast<std::uint8_t>(value / 6u);
}

constexpr std::array<Module22ListRecord, 9> kModule22FirmwareRecords{{
    {0x009f, 0x0059, 0x0000},
    {0x00a0, 0x005a, 0x0001},
    {0x00a6, 0x0061, 0x0002},
    {0x00a1, 0x005b, 0x0003},
    {0x00a2, 0x005c, 0x0004},
    {0x00a3, 0x005d, 0x0005},
    {0x00a4, 0x005f, 0x000a},
    {0x00a5, 0x005e, 0x000e},
    {0x00a7, 0x0062, 0x0006},
}};

} // namespace

std::span<const Module22ListRecord> module22_firmware_records() noexcept {
    return kModule22FirmwareRecords;
}

std::uint16_t normalize_module22_selection(const std::uint16_t code) noexcept {
    switch (code) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 8:
    case 10:
        return code;
    case 14:
        return 7;
    default:
        return 0;
    }
}

Module22SelectionGeometry compute_module22_selection_geometry(
    const std::uint16_t code) noexcept {
    Module22SelectionGeometry result{};
    result.capacity_value = 9;
    result.normalized_selection = code == 14
        ? 14
        : normalize_module22_selection(code);
    result.range_seed = static_cast<std::uint8_t>(code == 14
        ? 7
        : normalize_module22_selection(code));
    if (result.range_seed > 12 && code != 14) result.range_seed = 13;

    auto origin = static_cast<std::uint8_t>(divide_by_six(result.range_seed) * 6u);
    if (result.range_seed <= origin && origin != result.range_seed) {
        origin = static_cast<std::uint8_t>(origin - 6u);
    }
    result.range_origin = origin;
    result.offset_in_range = static_cast<std::uint8_t>(result.range_seed - origin);
    const auto remaining = static_cast<std::uint32_t>(result.capacity_value) - origin;
    result.visible_count = static_cast<std::uint8_t>(std::min(remaining, 6u));
    return result;
}

Module22SelectionPlan plan_module22_selection_change(
    Module22SelectionState state, const std::uint32_t target) {
    Module22SelectionPlan plan{};
    plan.state = state;
    if (target >= state.total) return plan;
    plan.accepted = true;

    if (target != state.current) {
        plan.changed = true;
        plan.state.previous = state.current;
        plan.state.current = static_cast<std::uint8_t>(target);
        const auto page_group = static_cast<std::uint32_t>(divide_by_six(state.page_start)) * 6u;
        plan.state.page_start = static_cast<std::uint8_t>(target + page_group);
        plan.actions.push_back({Module22SelectionEffect::notify_event, 0x171});
        plan.actions.push_back({Module22SelectionEffect::render_list, 0x0307ba78});
        plan.actions.push_back({Module22SelectionEffect::call_helper, 0x03065cfc});
    }

    plan.actions.push_back({Module22SelectionEffect::notify_event, 0x03e});
    return plan;
}

std::vector<Module22VisibleRow> plan_module22_visible_rows(
    const std::span<const Module22ListRecord> records,
    const std::uint8_t total, const std::uint8_t page_start,
    const std::uint8_t selected_row) {
    std::vector<Module22VisibleRow> rows;
    const auto remaining = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(total) - static_cast<std::uint16_t>(page_start));
    const auto count = std::min<std::uint16_t>(remaining, 6);
    rows.reserve(count);

    for (std::uint16_t row = 0; row < count; ++row) {
        const auto item_index = static_cast<std::uint16_t>(page_start + row);
        const bool selected = row == selected_row;
        const bool available = item_index < records.size();
        Module22VisibleRow planned{};
        planned.row = static_cast<std::uint8_t>(row);
        planned.item_index = item_index;
        planned.selected = selected;
        planned.record_available = available;
        if (selected) {
            planned.selected_indicator_icon = row == 0 ? 0x86 : row == 5 ? 0x88 : 0x85;
        }
        if (available) {
            const auto& record = records[item_index];
            planned.rendered_label_code = static_cast<std::uint16_t>(
                record.label_code + (selected ? 10u : 0u));
            planned.resource_code = record.resource_code;
        }
        rows.push_back(planned);
    }
    return rows;
}

} // namespace echo::recording
