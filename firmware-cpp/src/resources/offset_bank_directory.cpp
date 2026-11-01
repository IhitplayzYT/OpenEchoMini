#include "echo/resources/offset_bank_directory.hpp"

namespace echo::resources {
namespace {

bool read_u32_le(const std::span<const std::uint8_t> bytes,
                 const std::size_t offset,
                 std::uint32_t& value) noexcept {
    if (offset > bytes.size() || bytes.size() - offset < 4) return false;
    value = static_cast<std::uint32_t>(bytes[offset]) |
            (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
            (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
            (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
    return true;
}

bool read_offset(const std::span<const std::uint8_t> bytes,
                 const OffsetBankDirectory directory,
                 const std::size_t index,
                 std::size_t& value) noexcept {
    if (index >= directory.count) return false;
    std::uint32_t raw = 0;
    if (!read_u32_le(bytes, 2 + index * 4, raw)) return false;
    value = raw;
    return value <= bytes.size();
}

} // namespace

std::optional<OffsetBankDirectory> parse_offset_bank_directory(
    const std::span<const std::uint8_t> region) noexcept {
    if (region.size() < 2) return std::nullopt;
    const auto count = static_cast<std::size_t>(region[0]) |
                       (static_cast<std::size_t>(region[1]) << 8U);
    if (count == 0 || count > (region.size() - 2) / 4) return std::nullopt;

    const OffsetBankDirectory directory{count, 2 + count * 4};
    std::size_t previous = directory.header_bytes;
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t offset = 0;
        if (!read_offset(region, directory, i, offset) || offset < previous) {
            return std::nullopt;
        }
        previous = offset;
    }
    return directory;
}

std::optional<ByteRange> offset_bank_range(
    const std::span<const std::uint8_t> region,
    const std::size_t bank_index) noexcept {
    const auto directory = parse_offset_bank_directory(region);
    if (!directory || bank_index >= directory->count) return std::nullopt;

    std::size_t begin = 0;
    if (!read_offset(region, *directory, bank_index, begin)) return std::nullopt;

    auto end = region.size();
    if (bank_index + 1 < directory->count &&
        !read_offset(region, *directory, bank_index + 1, end)) {
        return std::nullopt;
    }
    if (end < begin) return std::nullopt;
    return ByteRange{begin, end - begin};
}

} // namespace echo::resources
