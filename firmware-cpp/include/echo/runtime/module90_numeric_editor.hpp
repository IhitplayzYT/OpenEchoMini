#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::runtime {

struct Module90DecimalField {
    std::uint64_t whole{};
    std::uint64_t fractional{};
    std::uint64_t decimal_scale{1};
    bool has_separator{};
    bool terminated{};
};

struct Module90ParsedFields {
    Module90DecimalField first;
    Module90DecimalField second;
    bool first_is_suppressed{};
};

struct Module90EditorBuffer {
    // Firmware rows are 22 bytes including the reserved index zero and 0xff
    // terminator slot. The parser consumes indices 1 through 21.
    std::array<std::uint8_t, 22> first_row{};
    std::array<std::uint8_t, 22> second_row{};
    std::uint8_t first_length{};
    std::uint8_t second_length{};
    std::uint8_t stage{};
    std::uint8_t operator_key{};
    std::uint8_t separator_guard{};
};

enum class Module90KeyResult {
    ignored,
    edited,
    conversion_requested,
    external_callback,
    rejected,
};

struct Module90Evaluation {
    std::uint64_t magnitude{};
    std::uint64_t decimal_scale{1};
    std::uint8_t display_precision{};
    bool negative{};
    bool range_error{};
};

// Decodes one module 90 byte row. Firmware rows reserve byte zero, scan
// bytes 1 through 21, use 0x12 as the whole/fraction separator, and stop at
// 0xff. Other bytes are accumulated as numeric values without validation.
// Accumulation deliberately wraps modulo 2^64, matching ARM word arithmetic.
bool parse_module90_decimal_row(std::span<const std::uint8_t> row,
                                Module90DecimalField& result);

// Parses the two rows consumed by FUN_0307b556. Mode 'X' suppresses the
// first-row parse; the second row is always read. This models values parsed
// during this call, not preexisting firmware state or later conversion.
bool parse_module90_fields(std::span<const std::uint8_t> first_row,
                           std::span<const std::uint8_t> second_row,
                           bool mode_x, Module90ParsedFields& result);

// Resolves the key byte in module 90's 5-row by 4-column touch table.
bool module90_touch_grid_key(std::size_t row, std::size_t column,
                             std::uint8_t& key);

// Applies the numeric-key, separator, zero-pair, prefix-toggle, operator, and
// conversion paths observed in FUN_0307bbf2. Screen-level touch/event
// callbacks remain caller-owned.
Module90KeyResult apply_module90_key(Module90EditorBuffer& state,
                                     std::uint8_t key) noexcept;

// Evaluates operator keys 0x0e through 0x11 from FUN_0307b734. Add/subtract
// follow the firmware's first-row sign handling; multiply/divide combine both
// row prefixes. A suppressed first parse requires the prior committed result,
// supplied as retained_first; its magnitude and scale are rescaled to the
// current common scale before the operation.
bool evaluate_module90_operation(const Module90ParsedFields& fields,
                                 std::uint8_t operator_key,
                                 bool first_negative,
                                 bool second_negative,
                                 Module90Evaluation& result,
                                 const Module90Evaluation* retained_first = nullptr) noexcept;

} // namespace echo::runtime
