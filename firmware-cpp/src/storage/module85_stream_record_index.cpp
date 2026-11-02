#include "echo/storage/module85_stream_record_index.hpp"

#include <cstddef>

namespace echo::storage {

namespace {

[[nodiscard]] std::uint32_t read_u32_le(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0])
         | (static_cast<std::uint32_t>(bytes[1]) << 8U)
         | (static_cast<std::uint32_t>(bytes[2]) << 16U)
         | (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

} // namespace

std::optional<Module85EntryHeader> decode_module85_entry_header(
    const std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < kModule85EntryHeaderBytes) {
        return std::nullopt;
    }
    Module85EntryHeader header{};
    for (std::size_t i = 0; i < header.words.size(); ++i) {
        header.words[i] = read_u32_le(bytes.data() + i * 4U);
    }
    return header;
}

std::optional<Module85RecordReadPlan> plan_module85_record_probe(
    const Module85StreamRecordTable& table,
    const Module85SearchBounds bounds) noexcept {
    if (bounds.lower >= bounds.upper || bounds.upper > table.record_count) {
        return std::nullopt;
    }

    const auto index = bounds.lower + ((bounds.upper - bounds.lower) >> 1U);
    const auto offset = table.record_base_offset
                      + index * kModule85FixedRecordBytes;
    return Module85RecordReadPlan{index, offset,
                                  kModule85FixedRecordBytes, table.stream_id};
}

Module85SearchUpdate update_module85_search_bounds(
    Module85SearchBounds& bounds, const std::uint32_t probe_index,
    const int compare_order) noexcept {
    if (bounds.lower >= bounds.upper || probe_index < bounds.lower
        || probe_index >= bounds.upper) {
        return Module85SearchUpdate::invalid;
    }
    if (compare_order > 0) {
        bounds.lower = probe_index + 1U;
        return Module85SearchUpdate::continue_search;
    }
    if (compare_order < 0) {
        bounds.upper = probe_index;
        return Module85SearchUpdate::continue_search;
    }
    return Module85SearchUpdate::found;
}

std::optional<int> compare_module85_zero_terminated_key(
    const std::span<const std::uint8_t> query,
    const std::span<const std::uint8_t> record) noexcept {
    const auto limit = query.size() < record.size() ? query.size() : record.size();
    for (std::size_t i = 0; i < limit; ++i) {
        const int difference = static_cast<int>(query[i])
                             - static_cast<int>(record[i]);
        if (difference != 0) {
            return difference;
        }
        if (query[i] == 0U) {
            return 0;
        }
    }
    return std::nullopt;
}

std::optional<int> compare_module85_record_key(
    const std::span<const std::uint8_t> query,
    const std::span<const std::uint8_t, kModule85FixedRecordBytes> record) noexcept {
    return compare_module85_zero_terminated_key(
        query, record.subspan<kModule85RecordKeyOffset>());
}

} // namespace echo::storage
