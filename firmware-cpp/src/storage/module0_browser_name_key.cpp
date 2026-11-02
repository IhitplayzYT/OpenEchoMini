#include "echo/storage/module0_browser_name_key.hpp"

namespace echo::storage {

namespace {

[[nodiscard]] std::size_t visible_length(
    const std::span<const char16_t> name) noexcept {
    std::size_t length = 0;
    while (length < name.size() && name[length] != u'\0') ++length;
    return length;
}

// FUN_030547fe copies a nonzero low byte and then a nonzero high byte. Its
// output count can exceed max_bytes by one when a two-byte unit straddles the
// bound, which is why callers provide a scratch buffer with one extra byte.
[[nodiscard]] std::size_t copy_firmware_bytes(
    std::span<std::uint8_t> output, const std::span<const char16_t> source,
    const std::size_t max_bytes) noexcept {
    if (max_bytes == 0U) return 0U;
    std::size_t written = 0;
    std::size_t input_index = 0;
    while (true) {
        const std::uint16_t unit = input_index < source.size()
            ? static_cast<std::uint16_t>(source[input_index]) : 0U;
        const auto low = static_cast<std::uint8_t>(unit & 0xffU);
        const auto high = static_cast<std::uint8_t>(unit >> 8U);
        if (low == 0U) {
            if (high == 0U) {
                output[written] = 0U;
                return written + 1U;
            }
            output[written++] = high;
        } else {
            output[written++] = low;
            if (high != 0U) output[written++] = high;
        }
        ++input_index;
        if (max_bytes <= written) return written;
    }
}

} // namespace

std::optional<Module0BrowserNameKey> format_module0_browser_name_key(
    const std::span<const char16_t> name,
    const std::uint32_t timer_tick_value) noexcept {
    Module0BrowserNameKey result{};
    result.bytes.fill(static_cast<std::uint8_t>(' '));

    const std::size_t length = visible_length(name);
    std::array<std::uint8_t, 5> prefix{};
    const std::size_t prefix_copy_count = copy_firmware_bytes(
        prefix, name.first(length), 4U);
    if (prefix_copy_count == 0U || prefix_copy_count >= 6U) return std::nullopt;
    for (std::size_t i = 0; i < prefix_copy_count; ++i) {
        result.bytes[i] = prefix[i];
    }
    for (std::size_t i = prefix_copy_count; i < 5U; ++i) {
        result.bytes[i] = static_cast<std::uint8_t>('~');
    }

    const std::uint32_t serial = timer_tick_value % 1000U;
    result.bytes[5] = static_cast<std::uint8_t>('0' + serial / 100U);
    result.bytes[6] = static_cast<std::uint8_t>('0' + (serial / 10U) % 10U);
    result.bytes[7] = static_cast<std::uint8_t>('0' + serial % 10U);

    std::size_t dot = length;
    while (dot > 0U) {
        --dot;
        if (name[dot] == u'.') {
            result.had_extension_separator = true;
            std::array<std::uint8_t, 4> extension{};
            const std::size_t copied = copy_firmware_bytes(
                extension, name.subspan(dot + 1U, length - dot - 1U), 3U);
            if (copied > 3U) return std::nullopt;
            for (std::size_t i = 0; i < copied; ++i) result.bytes[8U + i] = extension[i];
            break;
        }
    }

    if (result.had_extension_separator) {
        for (auto& byte : result.bytes) {
            if (byte >= static_cast<std::uint8_t>('a') &&
                byte <= static_cast<std::uint8_t>('z')) {
                byte = static_cast<std::uint8_t>(byte - ('a' - 'A'));
            }
        }
    }
    return result;
}

} // namespace echo::storage
