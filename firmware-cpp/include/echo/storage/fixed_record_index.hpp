#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace echo::storage {

enum class FixedRecordSearchStatus {
    found,
    not_found,
    invalid_table,
};

struct FixedRecordSearchResult {
    FixedRecordSearchStatus status = FixedRecordSearchStatus::invalid_table;
    std::size_t index{};
};

// Return a positive value when key sorts after record, negative when before,
// and zero for a match, matching the firmware's binary-search branch order.
using FixedRecordComparator = std::function<int(
    std::span<const std::uint8_t> key,
    std::span<const std::uint8_t> record)>;

class FixedRecordIndex {
public:
    static constexpr std::size_t record_size = 0x4c;

    explicit FixedRecordIndex(std::span<const std::uint8_t> records) noexcept;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] FixedRecordSearchResult find(
        std::span<const std::uint8_t> key,
        const FixedRecordComparator& compare) const;

private:
    std::span<const std::uint8_t> records_;
};

// Applies module 85's observed reversible byte transform in place.
void xor_payload_0f(std::span<std::uint8_t> payload) noexcept;

} // namespace echo::storage
