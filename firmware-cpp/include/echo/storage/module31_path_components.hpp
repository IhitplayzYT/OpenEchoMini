#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace echo::storage {

inline constexpr std::size_t module31_max_path_components = 5;
inline constexpr std::size_t module31_path_component_capacity = 0x3f;

struct Module31PathSplit {
    bool accepted{};
    std::array<std::u16string, module31_max_path_components> components{};
    std::uint8_t count{};
    // Diagnostic only: the firmware silently stops storing after 63 units.
    bool component_truncated{};
};

// Models FUN_0307bd52's slash/backslash splitting before browser lookup.
[[nodiscard]] Module31PathSplit split_module31_path(std::u16string_view path);

} // namespace echo::storage
