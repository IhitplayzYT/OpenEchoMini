#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace echo::audio {

enum class Module33CodecOperation : std::uint8_t {
    operation_3 = 0x03,
    operation_4 = 0x04,
    operation_5 = 0x05,
    operation_6 = 0x06,
    operation_7 = 0x07,
    operation_8 = 0x08,
    operation_10 = 0x0a,
    operation_12 = 0x0c,
    operation_14 = 0x0e,
    operation_15 = 0x0f,
    operation_17 = 0x11,
};

inline constexpr std::uint32_t kModule33CallbackTableAddress = 0x03046420;

struct Module33CodecInvocation {
    std::uint8_t selector{};
    std::uint8_t table_entry_index{};
    std::uint32_t table_slot_address{};
    std::uint32_t thumb_tagged_callback_pointer{};
    Module33CodecOperation operation{};
    std::uint32_t argument0{};
    std::uint32_t argument1{};
    std::uint32_t argument2{};
};

// Module 33 indexes its callback table by selector with four-byte entries.
// Selector 0xff suppresses dispatch. Operation numbers and argument values
// are preserved without assigning meanings to the selector or callback.
[[nodiscard]] inline std::optional<Module33CodecInvocation>
plan_module33_codec_invocation(
    std::uint8_t selector,
    Module33CodecOperation operation,
    std::uint32_t argument0 = 0,
    std::uint32_t argument1 = 0,
    std::uint32_t argument2 = 0) noexcept {
    constexpr std::array<std::uint32_t, 10> kCallbackEntries{
        0x0308c975, 0x0308c981, 0x0308c97d, 0x0308c975,
        0x0308c97d, 0x0308c97d, 0x0308c97d, 0x0308c97d,
        0x0308c97d, 0x0308c97d};
    if (selector == 0xff || selector >= kCallbackEntries.size()) {
        return std::nullopt;
    }
    return Module33CodecInvocation{
        selector, selector,
        kModule33CallbackTableAddress + static_cast<std::uint32_t>(selector) * 4U,
        kCallbackEntries[selector], operation,
        argument0, argument1, argument2};
}

} // namespace echo::audio
