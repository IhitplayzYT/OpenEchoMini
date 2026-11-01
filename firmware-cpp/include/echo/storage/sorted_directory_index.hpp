#pragma once

#include "echo/storage/directory_order.hpp"

#include <cstddef>
#include <vector>

namespace echo::storage {

// Stable in-memory equivalent of module 40's linked-list insertion helper.
// The caller owns any storage referenced by cached_name.
class SortedDirectoryIndex {
public:
    [[nodiscard]] std::size_t insert(DirectorySortKey key);
    [[nodiscard]] const std::vector<DirectorySortKey>& entries() const noexcept {
        return entries_;
    }

private:
    std::vector<DirectorySortKey> entries_;
};

} // namespace echo::storage
