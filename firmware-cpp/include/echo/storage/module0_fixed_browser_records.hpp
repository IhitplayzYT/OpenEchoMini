#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

enum class Module0FixedBrowserTable : std::uint8_t {
    table_one = 1,
    table_two = 2,
};

// Category codes observed in FUN_0305dfaa's browser dispatch. Category 10
// selects table one; categories 8 and 9 select table two.
[[nodiscard]] std::optional<Module0FixedBrowserTable>
module0_fixed_browser_table_for_category(std::uint16_t category) noexcept;

struct Module0FixedBrowserRecordLocation {
    // Byte offsets passed to the firmware's shared block-read helper.
    std::uint32_t index_word_offset{};
    std::uint32_t record_offset{};
};

struct Module0FixedBrowserRecord {
    std::array<std::uint8_t, 8> opaque_prefix{};
    std::uint32_t payload_index{};
    std::uint32_t kind_value{};
};

// The scanner reads 16 bytes and branches on the final word while using the
// preceding word as an index. The first eight bytes remain opaque.
[[nodiscard]] Module0FixedBrowserRecord
decode_module0_fixed_browser_record(
    std::span<const std::uint8_t, 16> bytes) noexcept;

// FUN_0305b93a scans each table until the first word is 0xffffffff. Its
// count paths accept kind 1/2 in table one and kind 1 in table two. The
// helper describes only those observed count rules; it does not classify the
// product meaning of either table or record kind.
[[nodiscard]] bool module0_fixed_browser_record_is_end_marker(
    const Module0FixedBrowserRecord& record) noexcept;
[[nodiscard]] bool module0_fixed_browser_record_contributes_to_count(
    Module0FixedBrowserTable table,
    const Module0FixedBrowserRecord& record) noexcept;

// Reconstructs the paired 16-bit index and 16-byte record offsets used by
// FUN_0305b120's scan-mode-1/2 branches. The supplied record_index is the
// value read from the index table. Arithmetic intentionally wraps at 32 bits,
// matching the observed firmware address expressions.
[[nodiscard]] std::optional<Module0FixedBrowserRecordLocation>
locate_module0_fixed_browser_record(Module0FixedBrowserTable table,
                                    std::uint32_t base_sector,
                                    std::uint32_t base_index,
                                    std::uint32_t cursor,
                                    std::uint16_t record_index) noexcept;

} // namespace echo::storage
