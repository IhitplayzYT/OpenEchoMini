#include "echo/storage/module0_fat_directory_filter.hpp"

namespace echo::storage {

bool module0_fat_directory_candidate_accepted(
    const std::span<const std::uint8_t> name_field,
    const std::uint8_t attributes, const std::uint8_t backend_selector,
    const std::uint8_t scan_mode) noexcept {
    if (name_field.empty()) return false;

    const auto first = name_field.front();
    if (first == 0xe5U || first == static_cast<std::uint8_t>('.')) return false;
    if ((attributes & 0x02U) != 0U || (attributes & 0x10U) == 0U) return false;

    // The firmware compares exactly 11 bytes against the padded RECORD field.
    constexpr std::uint8_t record_name[11] = {
        'R', 'E', 'C', 'O', 'R', 'D', ' ', ' ', ' ', ' ', ' '};
    if (backend_selector == 1U && scan_mode == 6U && name_field.size() >= 11U) {
        bool matches_record = true;
        for (std::size_t i = 0; i < 11U; ++i) {
            if (name_field[i] != record_name[i]) {
                matches_record = false;
                break;
            }
        }
        if (matches_record) return false;
    }
    return true;
}

} // namespace echo::storage
