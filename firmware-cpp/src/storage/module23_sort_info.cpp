#include "echo/storage/module23_sort_info.hpp"

namespace echo::storage {

Module23SortInfoPlan plan_module23_sort_info_check(
    const Module23SortInfoInput& input) noexcept {
    Module23SortInfoPlan plan{};
    if (!input.event_gate_matches || !input.state_gate_is_one) return plan;

    plan.gates_passed = true;
    plan.setup_helper_called = true;
    if (input.sort_mode == 10) {
        const auto storage_offset = input.storage_base << 9;
        const auto directory_sum = input.context_word_34 + input.context_word_38;
        plan.reads[0] = {
            storage_offset + 0x0a600000u + (directory_sum << 1) - 2u, 2};
        plan.reads[1] = {
            storage_offset + 0x08000000u +
                ((input.context_word_34 + input.first_read_index) << 4),
            0x10};
        plan.reads[2] = {
            storage_offset + (input.descriptor_block_index << 11), 4};
        plan.reads[3] = {plan.reads[2].byte_offset + 4u, 4};
        plan.read_count = 4;
        plan.pair_matches = input.expected_words == input.observed_words;
    } else {
        plan.pair_matches = input.expected_words[0] == input.context_word_34 &&
            input.expected_words[1] == input.context_word_38;
    }
    plan.return_value = plan.pair_matches ? 1 : 0;
    return plan;
}

Module23SortMenuGeometry compute_module23_sort_menu_geometry(
    const std::uint16_t selection_code) noexcept {
    const auto first_item = static_cast<std::uint8_t>(selection_code == 14 ? 6 : 0);
    const auto visible_rows = static_cast<std::uint8_t>(selection_code == 14 ? 3 : 6);
    const auto selected_row = static_cast<std::uint8_t>(
        selection_code >= 2 && selection_code <= 5 ? selection_code : 1);
    return {first_item, visible_rows, selected_row,
            static_cast<std::uint8_t>(first_item + selected_row)};
}

} // namespace echo::storage
