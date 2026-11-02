#include "echo/storage/module0_fixed_browser_records.hpp"

namespace echo::storage {

namespace {

[[nodiscard]] std::uint32_t read_u32_le(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

} // namespace

std::optional<Module0FixedBrowserTable>
module0_fixed_browser_table_for_category(const std::uint16_t category) noexcept {
    switch (category) {
    case 10U:
        return Module0FixedBrowserTable::table_one;
    case 8U:
    case 9U:
        return Module0FixedBrowserTable::table_two;
    default:
        return std::nullopt;
    }
}

Module0FixedBrowserRecord decode_module0_fixed_browser_record(
    const std::span<const std::uint8_t, 16> bytes) noexcept {
    Module0FixedBrowserRecord result{};
    for (std::size_t i = 0; i < result.opaque_prefix.size(); ++i) {
        result.opaque_prefix[i] = bytes[i];
    }
    result.payload_index = read_u32_le(bytes.data() + 8U);
    result.kind_value = read_u32_le(bytes.data() + 12U);
    return result;
}

bool module0_fixed_browser_record_is_end_marker(
    const Module0FixedBrowserRecord& record) noexcept {
    return read_u32_le(record.opaque_prefix.data()) == 0xffffffffU;
}

bool module0_fixed_browser_record_contributes_to_count(
    const Module0FixedBrowserTable table,
    const Module0FixedBrowserRecord& record) noexcept {
    switch (table) {
    case Module0FixedBrowserTable::table_one:
        return record.kind_value == 1U || record.kind_value == 2U;
    case Module0FixedBrowserTable::table_two:
        return record.kind_value == 1U;
    default:
        return false;
    }
}

std::optional<Module0FixedBrowserRecordLocation>
locate_module0_fixed_browser_record(const Module0FixedBrowserTable table,
                                    const std::uint32_t base_sector,
                                    const std::uint32_t base_index,
                                    const std::uint32_t cursor,
                                    const std::uint16_t record_index) noexcept {
    std::uint32_t index_table_base{};
    std::uint32_t record_table_base{};
    switch (table) {
    case Module0FixedBrowserTable::table_one:
        index_table_base = 0x0a600000U;
        record_table_base = 0x08000000U;
        break;
    case Module0FixedBrowserTable::table_two:
        index_table_base = 0x0a800000U;
        record_table_base = 0x08800000U;
        break;
    default:
        return std::nullopt;
    }

    const std::uint32_t partition_bytes = base_sector * 0x200U;
    const std::uint32_t index_position = base_index + cursor;
    const std::uint32_t record_position = base_index + record_index;
    return Module0FixedBrowserRecordLocation{
        index_table_base + partition_bytes + index_position * 2U,
        record_table_base + partition_bytes + record_position * 0x10U,
    };
}

} // namespace echo::storage
