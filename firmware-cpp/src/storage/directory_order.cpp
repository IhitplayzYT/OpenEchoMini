#include "echo/storage/directory_order.hpp"

#include <algorithm>
#include <cstddef>

namespace echo::storage {

DirectoryOrder DirectoryComparator::compare(const DirectorySortKey& first,
                                             const DirectorySortKey& second) noexcept {
    for (std::size_t field = 0; field < first.fields.size(); ++field) {
        if (first.fields[field] < second.fields[field]) return DirectoryOrder::before;
        if (first.fields[field] > second.fields[field]) return DirectoryOrder::after;
    }

    if (first.fields[3] == 0) return DirectoryOrder::same;

    constexpr auto max_name_units = std::size_t{0x80};
    const auto first_end = std::min({first.cached_name.find(u'\0'),
                                     first.cached_name.size(), max_name_units});
    const auto second_end = std::min({second.cached_name.find(u'\0'),
                                      second.cached_name.size(), max_name_units});
    const auto common = std::min(first_end, second_end);
    for (std::size_t i = 0; i < common; ++i) {
        if (first.cached_name[i] < second.cached_name[i]) return DirectoryOrder::before;
        if (first.cached_name[i] > second.cached_name[i]) return DirectoryOrder::after;
    }

    if (first_end < second_end) return DirectoryOrder::before;
    if (first_end > second_end) return DirectoryOrder::after;
    return DirectoryOrder::same;
}

} // namespace echo::storage
