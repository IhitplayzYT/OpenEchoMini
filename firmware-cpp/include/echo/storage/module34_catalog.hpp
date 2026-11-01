#pragma once

#include <cstdint>
#include <vector>

namespace echo::storage {

enum class Module34CatalogActionKind {
    search,
    refresh_entry,
    clear_event,
    set_selected_index,
    resolve_by_filename,
    read_catalog_index,
    read_catalog_record,
    fail_invalid_codec,
    fail_search,
};

struct Module34CatalogAction {
    Module34CatalogActionKind kind{};
    std::uint32_t argument{};
    std::uint32_t length{};
    std::uint32_t address{};
};

struct Module34CatalogInput {
    std::uint16_t format_selector{};
    std::uint32_t name_address{};
    std::uint32_t name_length{};
    std::uint32_t first_search_result{};
    std::uint8_t codec_before_refresh{};
    std::uint8_t codec_after_refresh{};
    std::uint8_t codec_after_special_refresh[4]{}; // after IDs 8, 9, 2, and 7
    // Search results consumed only when the corresponding codec remains active
    // after its special refresh: one for codecs 8/9, two for codecs 2/7.
    std::uint32_t special_search_results[4][2]{}; // one or two per codec stage
    // The assembly's codec-0xff/non-0xff-prior branch reaches a lookup tail
    // shared with codec 2. One comparison uses a global result not initialized
    // by this function, so both values remain explicit inputs.
    std::uint32_t codec_recovery_prior_result{};
    std::uint32_t codec_recovery_search_result{};
    bool file_number_is_zero{};
    std::uint16_t record_number{};
    std::uint16_t record_number_by_group{};
    std::uint32_t group_index{};
    std::uint32_t storage_group{};
    std::uint32_t selected_record{};
};

struct Module34CatalogPlan {
    std::uint32_t search_mode{};
    std::uint32_t result{}; // 0 on success, 0xffffffff on observed failure.
    std::vector<Module34CatalogAction> actions;
};

// Reconstructs the observable sequencing in FUN_0308c29c. Actual entry refresh,
// event dispatch, file lookup, and storage reads are firmware services and are
// represented as actions for a platform adapter to execute.
Module34CatalogPlan plan_module34_catalog(const Module34CatalogInput& input);

} // namespace echo::storage
