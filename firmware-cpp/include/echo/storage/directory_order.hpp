#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace echo::storage {

enum class DirectoryOrder : std::uint8_t {
    before = 0,
    same = 1,
    after = 2,
};

struct DirectorySortKey {
    std::array<std::uint16_t, 4> fields{};
    std::u16string_view cached_name;
};

class DirectoryComparator {
public:
    // Mirrors module 40's observed comparison: four unsigned 16-bit fields,
    // followed by UTF-16 code units when the fourth field is nonzero.
    [[nodiscard]] static DirectoryOrder compare(const DirectorySortKey& first,
                                                const DirectorySortKey& second) noexcept;
};

} // namespace echo::storage
