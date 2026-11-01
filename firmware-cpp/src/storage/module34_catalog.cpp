#include "echo/storage/module34_catalog.hpp"

namespace echo::storage {
namespace {
constexpr std::uint32_t failure = 0xffffffffu;
constexpr std::uint32_t event_id = 0x115u;

void add(Module34CatalogPlan& plan, Module34CatalogActionKind kind,
         std::uint32_t argument = 0, std::uint32_t length = 0,
         std::uint32_t address = 0) {
    plan.actions.push_back({kind, argument, length, address});
}
} // namespace

Module34CatalogPlan plan_module34_catalog(const Module34CatalogInput& in) {
    Module34CatalogPlan plan;
    plan.result = failure;
    plan.search_mode = in.format_selector == 10 ? 1u :
                       in.format_selector == 8 ? 2u : 3u;
    add(plan, Module34CatalogActionKind::search, plan.search_mode,
        in.name_length == 0 ? 0 : in.name_length - 1, in.name_address);
    if (in.first_search_result >= 9) {
        add(plan, Module34CatalogActionKind::fail_search, in.first_search_result);
        return plan;
    }

    add(plan, Module34CatalogActionKind::set_selected_index,
        in.first_search_result);
    add(plan, Module34CatalogActionKind::refresh_entry);
    add(plan, Module34CatalogActionKind::refresh_entry);
    const auto codec = in.codec_after_refresh;
    if (codec == 0xff) {
        if (in.codec_before_refresh == 0xff) {
            add(plan, Module34CatalogActionKind::fail_invalid_codec, codec);
            return plan;
        }
        add(plan, Module34CatalogActionKind::search, plan.search_mode,
            in.name_length == 0 ? 0 : in.name_length - 1,
            in.name_address);
        if (in.codec_recovery_prior_result > 8 ||
            in.codec_recovery_search_result > 8) {
            add(plan, Module34CatalogActionKind::fail_search,
                in.codec_recovery_search_result);
            return plan;
        }
    } else {
        constexpr std::uint8_t special_codecs[] = {8, 9, 2, 7};
        auto active_codec = codec;
        for (unsigned stage = 0; stage < 4; ++stage) {
            if (active_codec != special_codecs[stage]) continue;
            add(plan, Module34CatalogActionKind::clear_event, event_id);
            add(plan, Module34CatalogActionKind::refresh_entry);
            active_codec = in.codec_after_special_refresh[stage];
            if (active_codec != special_codecs[stage]) continue;
            const std::uint32_t count =
                (active_codec == 2 || active_codec == 7) ? 2u : 1u;
            for (std::uint32_t i = 0; i < count; ++i) {
                const auto found = in.special_search_results[stage][i];
                add(plan, Module34CatalogActionKind::search, plan.search_mode,
                    in.name_length == 0 ? 0 : in.name_length - 1,
                    in.name_address);
                if (found > 8) {
                    add(plan, Module34CatalogActionKind::fail_search, found);
                    return plan;
                }
            }
        }
    }

    if (in.file_number_is_zero || in.format_selector == 10 ||
        in.format_selector == 99) {
        add(plan, Module34CatalogActionKind::resolve_by_filename,
            in.name_address, in.name_length == 0 ? 0 : in.name_length - 1);
    } else {
        const std::uint32_t record_index =
            static_cast<std::uint32_t>(in.record_number) +
            static_cast<std::uint32_t>(in.record_number_by_group);
        add(plan, Module34CatalogActionKind::read_catalog_index,
            in.storage_group * 0x200u + record_index * 2u - 2u, 2);
        const std::uint32_t group = in.selected_record;
        add(plan, Module34CatalogActionKind::read_catalog_record,
            in.group_index * 0x200u + group * 0x800u + 0x400u, 0x80);
        add(plan, Module34CatalogActionKind::read_catalog_record,
            in.group_index * 0x200u + group * 0x800u + 0x500u, 0x100);
        add(plan, Module34CatalogActionKind::read_catalog_record,
            in.group_index * 0x200u + group * 0x800u + 0x600u, 0x100);
    }
    plan.result = 0;
    return plan;
}

} // namespace echo::storage
