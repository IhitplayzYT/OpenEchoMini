#include "echo/storage/module31_path_components.hpp"

namespace echo::storage {
namespace {

constexpr bool is_separator(const char16_t c) noexcept {
    return c == u'/' || c == u'\\';
}

} // namespace

Module31PathSplit split_module31_path(const std::u16string_view path) {
    Module31PathSplit result{};
    std::size_t component = 0;

    for (const char16_t character : path) {
        if (!is_separator(character)) {
            if (result.components[component].size() <
                module31_path_component_capacity) {
                result.components[component].push_back(character);
            } else {
                result.component_truncated = true;
            }
            continue;
        }

        auto& current = result.components[component];
        if (!current.empty()) {
            if (current != u".") {
                ++component;
                if (component >= module31_max_path_components) return {};
            }
            current.clear();
        }
    }

    if (component == 0 && result.components[0].empty()) return {};
    result.count = static_cast<std::uint8_t>(component + 1);
    result.accepted = true;
    return result;
}

} // namespace echo::storage
