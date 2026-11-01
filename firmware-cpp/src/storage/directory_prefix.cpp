#include "echo/storage/directory_prefix.hpp"

#include <cstddef>
#include <stdexcept>

namespace echo::storage {

DirectoryPrefixRecord DirectoryPrefixRecord::decode_runtime_node(
    std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 14) throw std::runtime_error("short directory runtime node");

    auto record = decode_prefix_bytes(bytes.subspan(4, 8));
    record.name_record_index = static_cast<std::uint16_t>(bytes[12]
        | (static_cast<std::uint16_t>(bytes[13]) << 8));
    return record;
}

DirectoryPrefixRecord DirectoryPrefixRecord::decode_prefix_bytes(
    std::span<const std::uint8_t> bytes, const std::uint16_t name_record_index) {
    if (bytes.size() < 8) throw std::runtime_error("short directory prefix bytes");

    DirectoryPrefixRecord record{};
    for (std::size_t i = 0; i < record.prefix.size(); ++i) {
        const auto offset = i * 2;
        auto unit = static_cast<std::uint16_t>(bytes[offset]
            | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8));
        if (unit >= u'a' && unit <= u'z') unit = static_cast<std::uint16_t>(unit - 0x20);
        record.prefix[i] = unit;
    }
    record.name_record_index = name_record_index;
    return record;
}

} // namespace echo::storage
