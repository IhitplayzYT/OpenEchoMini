#include "echo/runtime/module90_numeric_editor.hpp"
#include "echo/runtime/unsigned_divide.hpp"

namespace echo::runtime {

bool parse_module90_decimal_row(std::span<const std::uint8_t> row,
                                Module90DecimalField& result) {
    if (row.size() < 22) return false;

    Module90DecimalField parsed{};
    parsed.decimal_scale = 1;
    for (std::size_t index = 1; index <= 21; ++index) {
        const auto value = row[index];
        if (value == 0x12) {
            parsed.has_separator = true;
            continue;
        }
        if (value == 0xff) {
            parsed.terminated = true;
            break;
        }
        if (parsed.has_separator) {
            parsed.fractional = parsed.fractional * 10u + value;
            parsed.decimal_scale *= 10u;
        } else {
            parsed.whole = parsed.whole * 10u + value;
        }
    }
    result = parsed;
    return true;
}

bool parse_module90_fields(std::span<const std::uint8_t> first_row,
                           std::span<const std::uint8_t> second_row,
                           bool mode_x, Module90ParsedFields& result) {
    if ((!mode_x && first_row.size() < 22) || second_row.size() < 22) {
        return false;
    }

    Module90ParsedFields parsed{};
    parsed.first_is_suppressed = mode_x;
    if (!mode_x && !parse_module90_decimal_row(first_row, parsed.first)) {
        return false;
    }
    if (!parse_module90_decimal_row(second_row, parsed.second)) return false;
    result = parsed;
    return true;
}

bool module90_touch_grid_key(std::size_t row, std::size_t column,
                             std::uint8_t& key) {
    constexpr std::uint8_t keys[5][4] = {
        {0x0b, 0x0c, 0x0d, 0x0e},
        {0x07, 0x08, 0x09, 0x0f},
        {0x04, 0x05, 0x06, 0x10},
        {0x01, 0x02, 0x03, 0x11},
        {0x00, 0x12, 0x13, 0x14},
    };
    if (row >= 5 || column >= 4) return false;
    key = keys[row][column];
    return true;
}

namespace {
bool append_module90_digit(std::array<std::uint8_t, 22>& row,
                           std::uint8_t& length, std::uint8_t digit,
                           bool reject_duplicate_zero) noexcept {
    if (length > 13) return false;
    if (reject_duplicate_zero && digit == 0 && length == 1 && row[1] == 0) {
        return false;
    }
    const auto next_length = static_cast<std::uint8_t>(length + 1u);
    row[next_length] = digit;
    row[static_cast<std::size_t>(next_length) + 1u] = 0xff;
    length = next_length;
    return true;
}

bool append_module90_separator(std::array<std::uint8_t, 22>& row,
                               std::uint8_t& length,
                               std::uint8_t& guard) noexcept {
    if (length > 14 || guard != 0) return false;
    guard = 1;
    if (length == 0) {
        row[1] = 0;
        length = 2;
    } else {
        ++length;
    }
    row[length] = 0x12;
    row[static_cast<std::size_t>(length) + 1u] = 0xff;
    return true;
}

bool append_module90_zero_pair(std::array<std::uint8_t, 22>& row,
                               std::uint8_t& length) noexcept {
    if (length > 11 || row[1] == 0) return false;
    row[static_cast<std::size_t>(length) + 1u] = 0;
    row[static_cast<std::size_t>(length) + 2u] = 0;
    length = static_cast<std::uint8_t>(length + 2u);
    row[static_cast<std::size_t>(length) + 1u] = 0xff;
    return true;
}

Module90KeyResult edit_module90_separator(
    std::array<std::uint8_t, 22>& row, std::uint8_t& length,
    std::uint8_t& guard, std::uint8_t maximum_length) noexcept {
    if (length > maximum_length || row[1] == 0) {
        return Module90KeyResult::rejected;
    }
    if (row[static_cast<std::size_t>(length) + 1u] != 0xff) {
        return Module90KeyResult::ignored;
    }

    if (guard == 0) {
        guard = 1;
        if (length == 0) return Module90KeyResult::edited;
        if (length == 1) {
            const auto digit = row[1];
            const auto terminator = row[2];
            row[1] = 0;
            row[2] = 0x12;
            row[3] = 0;
            row[4] = digit;
            row[5] = terminator;
            length = 4;
            return Module90KeyResult::edited;
        }
        if (length == 2) {
            const auto first_digit = row[1];
            const auto second_digit = row[2];
            const auto terminator = row[3];
            row[1] = 0;
            row[2] = 0x12;
            row[3] = first_digit;
            row[4] = second_digit;
            row[5] = terminator;
            length = 4;
            return Module90KeyResult::edited;
        }

        row[static_cast<std::size_t>(length) + 2u] =
            row[static_cast<std::size_t>(length) + 1u];
        row[static_cast<std::size_t>(length) + 1u] = row[length];
        row[length] = row[static_cast<std::size_t>(length) - 1u];
        row[static_cast<std::size_t>(length) - 1u] = 0x12;
        ++length;
        return Module90KeyResult::edited;
    }

    // The firmware walks backward from the end marker and shifts bytes after
    // the existing separator two positions right, inserting two zero places.
    // Use a temporary row so a malformed row without a separator cannot leave
    // a partially shifted buffer.
    auto expanded = row;
    for (int index = length; index >= 0; --index) {
        const auto byte_index = static_cast<std::size_t>(index);
        const auto value = expanded[byte_index + 1u];
        if (value == 0x12) {
            expanded[byte_index + 2u] = 0;
            expanded[byte_index + 3u] = 0;
            row = expanded;
            length = static_cast<std::uint8_t>(length + 2u);
            return Module90KeyResult::edited;
        }
        expanded[byte_index + 3u] = value;
    }
    return Module90KeyResult::rejected;
}
} // namespace

Module90KeyResult apply_module90_key(Module90EditorBuffer& state,
                                     std::uint8_t key) noexcept {
    if (key < 10) {
        if (state.stage == 0x37) {
            if (!append_module90_digit(state.first_row, state.first_length,
                                       key, false)) {
                return Module90KeyResult::rejected;
            }
            state.stage = 0;
            return Module90KeyResult::edited;
        }
        if (state.stage == 0) {
            return append_module90_digit(state.first_row, state.first_length,
                                         key, true)
                       ? Module90KeyResult::edited
                       : Module90KeyResult::rejected;
        }
        if (state.stage == 1) {
            if (!append_module90_digit(state.second_row, state.second_length,
                                       key, false)) {
                return Module90KeyResult::rejected;
            }
            state.stage = 2;
            return Module90KeyResult::edited;
        }
        if (state.stage == 2) {
            return append_module90_digit(state.second_row, state.second_length,
                                         key, true)
                       ? Module90KeyResult::edited
                       : Module90KeyResult::rejected;
        }
        return Module90KeyResult::ignored;
    }

    if (key == 0x12) {
        if (state.stage == 0) {
            return append_module90_separator(state.first_row, state.first_length,
                                             state.separator_guard)
                       ? Module90KeyResult::edited
                       : Module90KeyResult::rejected;
        }
        if (state.stage == 2) {
            return append_module90_separator(state.second_row, state.second_length,
                                             state.separator_guard)
                       ? Module90KeyResult::edited
                       : Module90KeyResult::rejected;
        }
        return Module90KeyResult::ignored;
    }

    if (key == 0x13) {
        if (state.stage == 0) {
            return append_module90_zero_pair(state.first_row, state.first_length)
                       ? Module90KeyResult::edited
                       : Module90KeyResult::rejected;
        }
        if (state.stage == 2) {
            return append_module90_zero_pair(state.second_row, state.second_length)
                       ? Module90KeyResult::edited
                       : Module90KeyResult::rejected;
        }
        return Module90KeyResult::ignored;
    }

    if (key == 0x0c) {
        std::uint8_t* prefix = nullptr;
        if (state.stage == 0) prefix = &state.first_row[0];
        if (state.stage == 3) prefix = &state.second_row[0];
        if (prefix == nullptr) return Module90KeyResult::ignored;
        *prefix = *prefix == 0x0c ? 0 : 0x0c;
        return Module90KeyResult::edited;
    }

    if (key == 0x0b) return Module90KeyResult::external_callback;
    if (key == 0x0d) {
        if (state.stage == 0) {
            return edit_module90_separator(state.first_row, state.first_length,
                                           state.separator_guard, 10);
        }
        if (state.stage == 2) {
            return edit_module90_separator(state.second_row, state.second_length,
                                           state.separator_guard, 11);
        }
        return Module90KeyResult::ignored;
    }

    if (key >= 0x0e && key <= 0x11) {
        if (state.stage == 2) return Module90KeyResult::conversion_requested;
        if (state.stage == 0 || state.stage == 3) state.stage = 1;
        else if (state.stage != 1) return Module90KeyResult::ignored;
        state.operator_key = key;
        state.separator_guard = 0;
        return Module90KeyResult::edited;
    }

    if (key == 0x14 && state.stage == 2) {
        return Module90KeyResult::conversion_requested;
    }
    return Module90KeyResult::ignored;
}

namespace {
std::uint8_t decimal_places(std::uint64_t scale) noexcept {
    std::uint8_t places = 0;
    while (scale > 1 && scale % 10u == 0) {
        scale /= 10u;
        ++places;
    }
    return places;
}

bool normalize_module90_field(const Module90DecimalField& field,
                              std::uint64_t common_scale,
                              std::uint64_t& normalized) noexcept {
    if (field.decimal_scale == 0) return false;
    const auto scaled_fraction = firmware_unsigned_divide(
        field.fractional * common_scale, field.decimal_scale);
    normalized = field.whole * common_scale + scaled_fraction;
    return true;
}

FirmwareUnsignedDivision module90_divide(std::uint64_t numerator,
                                         std::uint64_t denominator) noexcept {
    return firmware_unsigned_divmod(numerator, denominator);
}
} // namespace

bool evaluate_module90_operation(const Module90ParsedFields& fields,
                                 std::uint8_t operator_key,
                                 bool first_negative,
                                 bool second_negative,
                                 Module90Evaluation& result,
                                 const Module90Evaluation* retained_first) noexcept {
    if (fields.first_is_suppressed && retained_first == nullptr) return false;
    if (operator_key < 0x0e || operator_key > 0x11) return false;

    const auto first_scale = fields.first_is_suppressed
                                 ? retained_first->decimal_scale
                                 : fields.first.decimal_scale;
    const auto common_scale = first_scale > fields.second.decimal_scale
                                  ? first_scale
                                  : fields.second.decimal_scale;
    std::uint64_t first = 0;
    std::uint64_t second = 0;
    if (fields.first_is_suppressed) {
        if (retained_first->decimal_scale == 0) return false;
        first = firmware_unsigned_divide(
            retained_first->magnitude * common_scale,
            retained_first->decimal_scale);
    } else if (!normalize_module90_field(fields.first, common_scale, first)) {
        return false;
    }
    if (!normalize_module90_field(fields.second, common_scale, second)) {
        return false;
    }

    const auto second_precision = decimal_places(fields.second.decimal_scale);
    const auto first_precision = fields.first_is_suppressed
                                     ? retained_first->display_precision
                                     : decimal_places(fields.first.decimal_scale);
    const auto precision = first_precision > second_precision
                               ? first_precision
                               : second_precision;
    Module90Evaluation evaluated{};
    evaluated.decimal_scale = common_scale;
    evaluated.display_precision = precision;

    if (operator_key == 0x10) {
        // The native subtraction branch consults the first row's sign marker.
        if (first_negative) {
            evaluated.magnitude = first + second;
            evaluated.negative = true;
        } else if (first >= second) {
            evaluated.magnitude = first - second;
        } else {
            evaluated.magnitude = second - first;
            evaluated.negative = true;
        }
        result = evaluated;
        return true;
    }

    if (operator_key == 0x11) {
        // The native addition branch also derives its sign from the first row.
        if (first_negative) {
            if (first >= second) {
                evaluated.magnitude = first - second;
                evaluated.negative = true;
            } else {
                evaluated.magnitude = second - first;
            }
        } else {
            evaluated.magnitude = first + second;
        }
        result = evaluated;
        return true;
    }

    evaluated.negative = first_negative != second_negative;
    if (operator_key == 0x0e) {
        if (second == 0) {
            evaluated.magnitude = 0x00005af3107a4000ull;
            evaluated.decimal_scale = 1;
            evaluated.display_precision = 0;
            evaluated.range_error = true;
            result = evaluated;
            return true;
        }

        constexpr std::array<std::uint64_t, 7> scales{
            1u, 10u, 100u, 1000u, 10000u, 100000u, 1000000u};
        for (std::size_t i = 0; i < scales.size(); ++i) {
            const auto divided = module90_divide(first * scales[i], second);
            if (divided.remainder == 0 || i + 1u == scales.size()) {
                evaluated.magnitude = divided.quotient;
                evaluated.decimal_scale = scales[i];
                evaluated.display_precision = i == 0
                                                  ? 0
                                                  : static_cast<std::uint8_t>(i + 1u);
                result = evaluated;
                return true;
            }
        }
    }

    // Multiplication produces two common-scale operands. The firmware reduces
    // to integer or one common scale when exact, otherwise it retains S squared.
    const auto maximum_precision = precision;
    const auto squared_scale = common_scale * common_scale;
    if (squared_scale > 0x00005af3b9a0f000ull || maximum_precision > 8) {
        evaluated.magnitude = 0x00005af346294650ull;
        evaluated.decimal_scale = 1;
        evaluated.display_precision = 0;
        evaluated.range_error = true;
        result = evaluated;
        return true;
    }

    const auto product = first * second;
    if (squared_scale != 0) {
        const auto integer_product = module90_divide(product, squared_scale);
        if (integer_product.remainder == 0) {
            evaluated.magnitude = integer_product.quotient;
            evaluated.decimal_scale = 1;
            evaluated.display_precision = 0;
            result = evaluated;
            return true;
        }
    }
    if (common_scale != 0) {
        const auto single_scale_product = module90_divide(product, common_scale);
        if (single_scale_product.remainder == 0) {
            evaluated.magnitude = single_scale_product.quotient;
            evaluated.decimal_scale = common_scale;
            result = evaluated;
            return true;
        }
    }
    evaluated.magnitude = product;
    evaluated.decimal_scale = squared_scale;
    evaluated.display_precision = maximum_precision == 0
                                      ? 0
                                      : static_cast<std::uint8_t>(
                                            maximum_precision * 2u - 1u);
    result = evaluated;
    return true;
}

} // namespace echo::runtime
