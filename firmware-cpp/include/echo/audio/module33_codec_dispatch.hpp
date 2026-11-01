#pragma once

#include <array>
#include <cstdint>

namespace echo::audio {

// Numeric operation values are preserved from module 33's indirect callback
// wrappers. Their format-specific meanings have not yet been recovered.
enum class Module33CodecOperation : std::uint8_t {
    op3 = 3,
    op4 = 4,
    op5 = 5,
    op6 = 6,
    op7 = 7,
    op8 = 8,
    op10 = 10,
    op12 = 12,
    op14 = 14,
    op15 = 15,
    op17 = 17,
};

struct Module33CodecDispatchPlan {
    std::uint8_t selector{};
    Module33CodecOperation operation{};
    std::array<std::uint32_t, 2> arguments{};
    bool callback_present{};
};

// Mirrors the wrappers around module 33's callback table. Selector 0xff skips
// the indirect call. The adapter owns table lookup and invokes the chosen
// callback with operation, arguments[0], arguments[1], and trailing zero.
[[nodiscard]] Module33CodecDispatchPlan plan_module33_codec_dispatch(
    std::uint8_t selector,
    Module33CodecOperation operation,
    std::uint32_t argument0 = 0,
    std::uint32_t argument1 = 0) noexcept;

} // namespace echo::audio
