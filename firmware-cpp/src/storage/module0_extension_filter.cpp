#include "echo/storage/module0_extension_filter.hpp"

namespace echo::storage {

bool module0_extension_filter_accepts(
    const std::string_view extension,
    const std::span<const char> packed_filter,
    const std::uint8_t file_flags) noexcept {
    if (packed_filter.empty()) return false;
    if (packed_filter.front() == '*') return true;

    if (extension.size() < 3U) return false;
    for (std::size_t offset = 0; offset + 3U <= packed_filter.size();
         offset += 3U) {
        if (packed_filter[offset] == '\0') break;
        if (extension[0] == packed_filter[offset] &&
            extension[1] == packed_filter[offset + 1U] &&
            extension[2] == packed_filter[offset + 2U]) {
            return (file_flags & 0x02U) == 0;
        }
    }
    return false;
}

} // namespace echo::storage
