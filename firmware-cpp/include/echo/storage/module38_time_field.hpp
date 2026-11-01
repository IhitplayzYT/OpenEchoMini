#pragma once

#include <cstdint>

namespace echo::storage {

struct Module38TimeField {
    std::uint8_t minutes{};
    std::uint8_t seconds{};
    std::uint8_t frames{};
};

struct Module38TimeValue {
    std::uint32_t milliseconds{};
    std::uint8_t frames_used{};
    bool frame_component_clamped{};
};

// Models the time-like record arithmetic in FUN_0307b9f2. The caller supplies
// already-decoded components; line syntax and digit validation are separate.
[[nodiscard]] Module38TimeValue convert_module38_time_field(
    Module38TimeField field) noexcept;

} // namespace echo::storage
