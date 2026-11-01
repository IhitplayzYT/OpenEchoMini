#include "echo/resources/localized_ui_strings.hpp"

namespace echo::resources {
namespace {

constexpr std::size_t kLocaleCount = 21;
constexpr std::size_t kRecordsPerBank = 453;
constexpr std::size_t kRecordBytes = 129 * 2;
constexpr std::size_t kTextOffsetBytes = 29 * 2;
constexpr std::size_t kTextCapacityUnits = 100;

std::uint16_t read_u16_le(const std::span<const std::uint8_t> bytes,
                          const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset]) |
           (static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

} // namespace

std::optional<ByteRange> localized_ui_text_range(
    const std::span<const std::uint8_t> region,
    const FirmwareLocale locale,
    const std::size_t record_index) noexcept {
    const auto locale_index = static_cast<std::size_t>(locale);
    if (locale_index >= kLocaleCount || record_index >= kRecordsPerBank) {
        return std::nullopt;
    }

    const auto bank = offset_bank_range(region, locale_index);
    if (!bank || bank->length != kRecordBytes * kRecordsPerBank) {
        return std::nullopt;
    }

    const auto text_start = bank->offset + record_index * kRecordBytes +
                            kTextOffsetBytes;
    const auto text_end_limit = text_start + kTextCapacityUnits * 2;
    if (text_end_limit > region.size()) return std::nullopt;

    for (std::size_t cursor = text_start; cursor < text_end_limit; cursor += 2) {
        const auto value = read_u16_le(region, cursor);
        if (value == 0 || value == 0xffff) {
            return ByteRange{text_start, cursor - text_start};
        }
    }
    return std::nullopt;
}

} // namespace echo::resources
