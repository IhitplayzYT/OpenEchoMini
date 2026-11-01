#include "echo/storage/sorted_directory_index.hpp"

namespace echo::storage {

std::size_t SortedDirectoryIndex::insert(DirectorySortKey key) {
    std::size_t position = 0;
    while (position < entries_.size() &&
           DirectoryComparator::compare(entries_[position], key) != DirectoryOrder::after) {
        ++position;
    }
    entries_.insert(entries_.begin() + static_cast<std::ptrdiff_t>(position), key);
    return position;
}

} // namespace echo::storage
