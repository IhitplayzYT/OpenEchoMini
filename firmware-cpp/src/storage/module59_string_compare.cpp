#include "echo/storage/module59_string_compare.hpp"

namespace echo::storage {
namespace {

bool ascii_letter(const std::uint8_t byte) {
    return (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z');
}

std::uint8_t fold_ascii(const std::uint8_t byte) {
    return byte >= 'A' && byte <= 'Z'
        ? static_cast<std::uint8_t>(byte + ('a' - 'A'))
        : byte;
}

bool skip_byte(const std::uint8_t byte, const Module59CompareOptions& options) {
    if (options.skip_high_bytes && byte >= 0x80) return true;
    return options.skip_non_ascii_letters && !ascii_letter(byte);
}

struct ReadByte {
    std::uint8_t value{};
    bool terminated{};
};

ReadByte read_byte(const std::span<const std::uint8_t> bytes, const std::size_t index) {
    if (index >= bytes.size()) return {0, false};
    if (bytes[index] == 0) return {0, true};
    return {bytes[index], false};
}

} // namespace

Module59CompareResult compare_module59_byte_strings(
    const std::optional<std::span<const std::uint8_t>> lhs,
    const std::optional<std::span<const std::uint8_t>> rhs,
    const Module59CompareOptions options) {
    Module59CompareResult result{};
    if (!lhs && !rhs) return result;
    if (!lhs) { result.order = -1; return result; }
    if (!rhs) { result.order = 1; return result; }

    result.equal_count_initialized = true;
    std::size_t lhs_index = 0;
    std::size_t rhs_index = 0;
    std::size_t effective_limit = options.compared_byte_limit;
    constexpr auto unlimited = static_cast<std::size_t>(-1);

    for (;;) {
        if (effective_limit != unlimited && result.equal_byte_count >= options.compared_byte_limit) {
            result.order = 0;
            return result;
        }

        auto left = read_byte(*lhs, lhs_index);
        result.lhs_terminated = left.terminated;
        while (left.value != 0 && skip_byte(left.value, options)) {
            ++lhs_index;
            if (effective_limit != unlimited) ++effective_limit;
            left = read_byte(*lhs, lhs_index);
            result.lhs_terminated = left.terminated;
        }

        auto right = read_byte(*rhs, rhs_index);
        result.rhs_terminated = right.terminated;
        while (right.value != 0 && skip_byte(right.value, options)) {
            ++rhs_index;
            right = read_byte(*rhs, rhs_index);
            result.rhs_terminated = right.terminated;
        }

        const auto left_value = options.ascii_case_insensitive ? fold_ascii(left.value) : left.value;
        const auto right_value = options.ascii_case_insensitive ? fold_ascii(right.value) : right.value;
        if (left_value != right_value) {
            result.order = left_value < right_value ? -1 : 1;
            return result;
        }
        if (left_value == 0) return result;

        ++result.equal_byte_count;
        ++lhs_index;
        ++rhs_index;
    }
}

} // namespace echo::storage
