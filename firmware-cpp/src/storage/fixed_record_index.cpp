#include "echo/storage/fixed_record_index.hpp"

namespace echo::storage {

FixedRecordIndex::FixedRecordIndex(
    const std::span<const std::uint8_t> records) noexcept
    : records_(records) {}

std::size_t FixedRecordIndex::size() const noexcept {
    return records_.size() / record_size;
}

FixedRecordSearchResult FixedRecordIndex::find(
    const std::span<const std::uint8_t> key,
    const FixedRecordComparator& compare) const {
    if (!compare || records_.size() % record_size != 0) {
        return {FixedRecordSearchStatus::invalid_table, 0};
    }

    std::size_t lower = 0;
    std::size_t upper = size();
    while (lower < upper) {
        const auto middle = lower + (upper - lower) / 2;
        const auto record = records_.subspan(middle * record_size, record_size);
        const auto order = compare(key, record);
        if (order > 0) {
            lower = middle + 1;
        } else if (order < 0) {
            upper = middle;
        } else {
            return {FixedRecordSearchStatus::found, middle};
        }
    }
    return {FixedRecordSearchStatus::not_found, 0};
}

void xor_payload_0f(const std::span<std::uint8_t> payload) noexcept {
    for (auto& byte : payload) {
        byte ^= 0x0fU;
    }
}

} // namespace echo::storage
