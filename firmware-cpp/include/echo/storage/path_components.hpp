#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace echo::storage {

struct FirmwarePathComponents {
    static constexpr std::size_t max_components = 5;
    static constexpr std::size_t component_capacity = 64;

    std::array<std::array<char16_t, component_capacity>, max_components> values{};
    std::size_t count{};
};

// Splits the bounded UTF-16 path format observed in module 23. Empty path
// components and single-dot components are skipped; two-dot components remain.
// Returns false when the firmware's five-component limit would be exceeded.
[[nodiscard]] bool split_firmware_path(
    std::span<const char16_t> nul_terminated_path,
    FirmwarePathComponents& output) noexcept;

} // namespace echo::storage
