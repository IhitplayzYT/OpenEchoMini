#include "echo/audio/vorbis_floor1.hpp"

#include <algorithm>
#include <limits>

namespace echo::audio {
namespace {

bool read_lsb(std::span<const std::uint8_t> packet, std::size_t& bit_offset,
              std::uint8_t width, std::uint32_t& value) noexcept {
    if (width > 32 || bit_offset > packet.size() * 8 ||
        width > packet.size() * 8 - bit_offset) {
        return false;
    }
    value = 0;
    for (std::uint8_t i = 0; i < width; ++i, ++bit_offset) {
        const auto byte_index = bit_offset / 8;
        const auto bit_index = static_cast<unsigned>(bit_offset % 8);
        value |= static_cast<std::uint32_t>((packet[byte_index] >> bit_index) & 1U) << i;
    }
    return true;
}

std::uint8_t ilog(std::uint32_t value) noexcept {
    std::uint8_t width = 0;
    while (value != 0) {
        ++width;
        value >>= 1U;
    }
    return width;
}

} // namespace

std::optional<VorbisFloor1Packet> decode_vorbis_floor1_packet(
    std::span<const std::uint8_t> packet,
    const VorbisFloor1Setup& setup,
    const VorbisFloorCodebookDecode& decode_scalar) {
    if (setup.y_range == 0 || setup.x_list.size() < 2 ||
        setup.low_neighbor.size() != setup.x_list.size() ||
        setup.high_neighbor.size() != setup.x_list.size() ||
        setup.x_list[0] >= setup.x_list[1] || !decode_scalar) {
        return std::nullopt;
    }

    std::size_t bit_offset = 0;
    std::uint32_t value = 0;
    if (!read_lsb(packet, bit_offset, 1, value)) return std::nullopt;

    VorbisFloor1Packet result;
    result.present = value != 0;
    result.y.assign(setup.x_list.size(), 0);
    result.active.assign(setup.x_list.size(), false);
    if (!result.present) {
        result.bits_consumed = bit_offset;
        return result;
    }

    const auto y_width = ilog(static_cast<std::uint32_t>(setup.y_range) - 1U);
    std::uint32_t y0 = 0;
    std::uint32_t y1 = 0;
    if (!read_lsb(packet, bit_offset, y_width, y0) ||
        !read_lsb(packet, bit_offset, y_width, y1) ||
        y0 >= setup.y_range || y1 >= setup.y_range) {
        return std::nullopt;
    }
    result.y[0] = static_cast<std::uint16_t>(y0);
    result.y[1] = static_cast<std::uint16_t>(y1);
    result.active[0] = true;
    result.active[1] = true;

    std::size_t post_count = 2;
    for (const auto class_index : setup.partition_classes) {
        if (class_index >= setup.classes.size()) return std::nullopt;
        const auto dimensions = setup.classes[class_index].dimensions;
        if (dimensions == 0 || dimensions > 8 ||
            dimensions > setup.x_list.size() - post_count) {
            return std::nullopt;
        }
        post_count += dimensions;
    }
    if (post_count != setup.x_list.size()) return std::nullopt;

    std::size_t post = 2;
    for (const auto class_index : setup.partition_classes) {
        const auto& cls = setup.classes[class_index];
        if (cls.subclass_bits > 8 || cls.subclass_books.size() != (1U << cls.subclass_bits) ||
            (cls.subclass_bits != 0 && !cls.masterbook)) {
            return std::nullopt;
        }

        std::uint32_t master_symbol = 0;
        if (cls.subclass_bits != 0) {
            const auto before = bit_offset;
            const auto decoded = decode_scalar(*cls.masterbook, packet, bit_offset);
            if (!decoded || *decoded < 0 || bit_offset < before ||
                bit_offset > packet.size() * 8) {
                return std::nullopt;
            }
            master_symbol = static_cast<std::uint32_t>(*decoded);
        }

        const auto mask = cls.subclass_bits == 0 ? 0U : (1U << cls.subclass_bits) - 1U;
        for (std::size_t d = 0; d < cls.dimensions; ++d, ++post) {
            const auto selector = master_symbol & mask;
            master_symbol >>= cls.subclass_bits;
            std::int32_t decoded_value = 0;
            const auto book = cls.subclass_books[selector];
            if (book != 0xff) {
                const auto before = bit_offset;
                const auto decoded = decode_scalar(book, packet, bit_offset);
                if (!decoded || *decoded < 0 || bit_offset < before ||
                    bit_offset > packet.size() * 8) {
                    return std::nullopt;
                }
                decoded_value = *decoded;
            }
            if (decoded_value > std::numeric_limits<std::uint16_t>::max()) {
                return std::nullopt;
            }
            result.y[post] = static_cast<std::uint16_t>(decoded_value);
        }
    }

    for (std::size_t i = 2; i < result.y.size(); ++i) {
        const auto low = setup.low_neighbor[i];
        const auto high = setup.high_neighbor[i];
        if (low >= i || high >= i || low >= setup.x_list.size() ||
            high >= setup.x_list.size() || setup.x_list[low] >= setup.x_list[high]) {
            return std::nullopt;
        }

        const auto x = setup.x_list[i];
        if (x <= setup.x_list[low] || x >= setup.x_list[high]) return std::nullopt;
        const auto x_span = static_cast<std::int32_t>(setup.x_list[high] - setup.x_list[low]);
        const auto y_span = static_cast<std::int32_t>(result.y[high]) - result.y[low];
        const auto predicted = static_cast<std::int32_t>(result.y[low]) +
            y_span * static_cast<std::int32_t>(x - setup.x_list[low]) / x_span;
        const auto low_room = predicted;
        const auto high_room = static_cast<std::int32_t>(setup.y_range) - predicted;
        if (low_room < 0 || high_room < 0) return std::nullopt;
        const auto room = 2 * std::min(low_room, high_room);
        const auto coded = static_cast<std::int32_t>(result.y[i]);

        if (coded == 0) {
            result.y[i] = static_cast<std::uint16_t>(predicted);
            result.active[i] = false;
            continue;
        }

        std::int32_t unwrapped = 0;
        if (coded >= room) {
            unwrapped = high_room > low_room
                ? coded - low_room + predicted
                : predicted - high_room - (coded - room);
        } else if ((coded & 1) != 0) {
            unwrapped = predicted - ((coded + 1) / 2);
        } else {
            unwrapped = predicted + coded / 2;
        }
        if (unwrapped < 0 || unwrapped >= setup.y_range) return std::nullopt;
        result.y[i] = static_cast<std::uint16_t>(unwrapped);
        result.active[i] = true;
    }

    result.bits_consumed = bit_offset;
    return result;
}

} // namespace echo::audio
