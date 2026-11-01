#include "echo/storage/module59_record_transform.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace echo::storage {
namespace {

constexpr std::array<std::uint8_t, 512> transform_tables{
#include "module59_transform_tables.inc"
};

std::uint16_t read_u16(const std::span<const std::uint8_t> bytes, const std::size_t at) {
    return static_cast<std::uint16_t>(bytes[at]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[at + 1]) << 8);
}

std::uint32_t read_u32(const std::span<const std::uint8_t> bytes, const std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at]) |
           (static_cast<std::uint32_t>(bytes[at + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[at + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[at + 3]) << 24);
}

std::uint8_t add8(const std::uint32_t a, const std::uint32_t b) {
    return static_cast<std::uint8_t>(a + b);
}

std::uint8_t swap_nibbles(const std::uint8_t value) {
    return static_cast<std::uint8_t>((value << 4) | (value >> 4));
}

std::uint8_t rotate_left_six(const std::uint8_t value) {
    return static_cast<std::uint8_t>((value << 6) | (value >> 2));
}

std::uint8_t header_seed(const std::span<const std::uint8_t> record,
                         const std::uint32_t key_offset) {
    if (key_offset == 0) return 0;
    const auto bucket = (key_offset & 0x1ffffu) >> 9;
    const auto folded_index = (key_offset & 0x1ffu) >> 1;
    const auto key_high = static_cast<std::uint8_t>(bucket);
    const auto key_low = static_cast<std::uint8_t>(folded_index);
    const auto sum_4_8_12 = add8(add8(record[4], record[8]), record[12]);

    if (bucket >= 1 && bucket <= 30) {
        return static_cast<std::uint8_t>(add8(add8(record[4], record[0]),
                                              add8(record[8], record[12])) ^ key_low);
    }
    if (bucket >= 41 && bucket <= 70) {
        return static_cast<std::uint8_t>(add8(key_high, key_low) ^
               add8(record[4], record[0]) ^ add8(record[8], record[12]));
    }
    if (bucket >= 81 && bucket <= 110) {
        return static_cast<std::uint8_t>(key_high ^ key_low ^
               add8(record[4], record[0]) ^ record[8]);
    }
    if (bucket >= 121 && bucket <= 150) {
        return static_cast<std::uint8_t>(add8(record[0], record[8]) ^ bucket ^
               folded_index ^ sum_4_8_12);
    }
    if (bucket >= 161 && bucket <= 190) {
        return static_cast<std::uint8_t>(add8(bucket, record[0]) ^ record[8] ^
               folded_index ^ sum_4_8_12);
    }
    return static_cast<std::uint8_t>(record[8] ^
           add8(add8(record[0], key_high), key_low) ^ sum_4_8_12);
}

std::uint8_t table_at(const std::uint8_t mode, const std::uint8_t index) {
    return transform_tables[static_cast<std::size_t>(mode) * 256 + index];
}

std::uint8_t transform_table3_value(const std::uint8_t value) {
    const auto high = static_cast<std::uint8_t>(value >> 4);
    return static_cast<std::uint8_t>(((high & 3u) << 6) |
        ((((value & 0x0fu) << 4) | high) >> 2));
}

} // namespace

std::int32_t transform_module59_record(const std::span<std::uint8_t> bytes,
                                       const std::int32_t length,
                                       const std::span<const std::uint8_t> record,
                                       const std::uint32_t stream_offset,
                                       const std::uint32_t key_offset) {
    if (bytes.data() == nullptr || length == 0 || record.data() == nullptr || record.size() < 16) {
        return 0;
    }
    if (length > 0 && static_cast<std::size_t>(length) > bytes.size()) return 0;

    const auto type = record[0];
    if (type != 3 && type != 5 && type != 6) return length;

    const auto mode = record[4];
    if ((type == 3 && mode != 3 && mode != 4) ||
        ((type == 5 || type == 6) && mode >= 2)) return 0;

    const auto start = read_u32(record, 12);
    if (type == 6 && std::bit_cast<std::int32_t>(start) >
                         std::bit_cast<std::int32_t>(stream_offset)) return 0;
    if (length < 1) return length;

    const auto seed = header_seed(record, key_offset);
    const auto count = static_cast<std::size_t>(length);

    if (type == 3) {
        const auto mode_index = static_cast<std::uint8_t>(read_u32(record, 8));
        if (mode_index >= 2) return 0;
        std::uint8_t index{};
        if (mode == 3) {
            index = static_cast<std::uint8_t>(((read_u32(record, 8) & 0xffffu) >> 8) ^
                       ~(static_cast<std::uint32_t>(read_u16(record, 12) >> 3)));
        } else {
            const auto rot = swap_nibbles(record[12]);
            index = static_cast<std::uint8_t>(rot ^ ((read_u32(record, 8) & 0xffffu) >> 8));
        }
        const auto mask = static_cast<std::uint8_t>(transform_table3_value(table_at(mode_index, index)) ^ seed);
        for (std::size_t i = 0; i < count; ++i) bytes[i] ^= mask;
        return length;
    }

    if (type == 5) {
        for (std::size_t i = 0; i < count; ++i) {
            const auto key_byte = static_cast<std::uint8_t>(stream_offset + i);
            const auto index = static_cast<std::uint8_t>(swap_nibbles(key_byte) ^ record[8]);
            const auto mask = rotate_left_six(swap_nibbles(table_at(mode, index)));
            bytes[i] ^= static_cast<std::uint8_t>(mask ^ seed);
        }
        return length;
    }

    for (std::size_t i = 0; i < count; ++i) {
        const auto phase = static_cast<std::uint32_t>((((stream_offset - start) & 3u) + i) & 3u);
        std::uint8_t index{};
        if (phase == 0) index = static_cast<std::uint8_t>(record[8] ^ (start >> 2));
        else if (phase == 1) index = static_cast<std::uint8_t>(record[8] ^ (start >> 1));
        else if (phase == 2) index = static_cast<std::uint8_t>(~start ^ record[8]);
        else index = static_cast<std::uint8_t>(start ^ record[8]);
        const auto mask = rotate_left_six(swap_nibbles(table_at(mode, index)));
        bytes[i] ^= static_cast<std::uint8_t>(mask ^ seed);
    }
    return length;
}

} // namespace echo::storage
