#include "echo/storage/module38_time_field.hpp"

namespace echo::storage {

Module38TimeValue convert_module38_time_field(
    const Module38TimeField field) noexcept {
    constexpr std::uint8_t max_frames = 74;
    const auto frames = field.frames > max_frames ? max_frames : field.frames;
    const auto whole_seconds =
        static_cast<std::uint32_t>(field.minutes) * 60u + field.seconds;
    return {
        whole_seconds * 1000u + static_cast<std::uint32_t>(frames) * 1000u / 74u,
        frames,
        field.frames > max_frames,
    };
}

} // namespace echo::storage
