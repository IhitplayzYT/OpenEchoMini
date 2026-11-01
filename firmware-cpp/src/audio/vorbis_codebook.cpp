#include "echo/audio/vorbis_codebook.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace echo::audio {
namespace {

class LsbBitReader {
public:
    explicit LsbBitReader(std::span<const std::uint8_t> bytes) noexcept : bytes_(bytes) {}

    [[nodiscard]] bool read(std::uint8_t width, std::uint32_t& value) noexcept {
        if (width > 32 || bit_offset_ > bytes_.size() * 8 ||
            width > bytes_.size() * 8 - bit_offset_) {
            return false;
        }
        value = 0;
        for (std::uint8_t i = 0; i < width; ++i, ++bit_offset_) {
            const auto byte_index = bit_offset_ / 8;
            const auto bit_index = static_cast<unsigned>(bit_offset_ % 8);
            value |= static_cast<std::uint32_t>((bytes_[byte_index] >> bit_index) & 1U) << i;
        }
        return true;
    }

    [[nodiscard]] std::size_t bit_offset() const noexcept { return bit_offset_; }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t bit_offset_{};
};

std::uint8_t ilog(std::uint32_t value) noexcept {
    std::uint8_t width = 0;
    while (value != 0) {
        ++width;
        value >>= 1U;
    }
    return width;
}

std::uint32_t lookup1_values(std::uint32_t entries, std::uint16_t dimensions) noexcept {
    std::uint32_t low = 0;
    std::uint32_t high = entries;
    while (low < high) {
        const auto mid = low + (high - low + 1U) / 2U;
        std::uint64_t power = 1;
        for (std::uint16_t i = 0; i < dimensions && power <= entries; ++i) power *= mid;
        if (power <= entries) low = mid;
        else high = mid - 1U;
    }
    return low;
}

bool valid_code_lengths(std::span<const std::uint8_t> lengths) noexcept {
    std::array<std::uint32_t, 33> counts{};
    for (const auto length : lengths) {
        if (length > 32) return false;
        if (length != 0) ++counts[length];
    }
    std::int64_t available = 1;
    for (std::size_t length = 1; length < counts.size(); ++length) {
        available = available * 2 - counts[length];
        if (available < 0) return false;
    }
    return true;
}

bool build_decode_tree(VorbisCodebookSetup& setup, std::size_t max_nodes) {
    if (max_nodes == 0) return false;
    setup.decode_tree.clear();
    setup.decode_tree.emplace_back();
    std::array<std::uint32_t, 33> markers{};

    for (std::uint32_t symbol = 0; symbol < setup.entries; ++symbol) {
        const auto length = setup.codeword_lengths[symbol];
        if (length == 0) continue;
        const auto code = markers[length];
        std::int32_t node = 0;
        for (std::uint8_t remaining = length; remaining > 0; --remaining) {
            const auto branch = static_cast<std::uint8_t>((code >> (remaining - 1U)) & 1U);
            const bool leaf = remaining == 1;
            auto child = setup.decode_tree[static_cast<std::size_t>(node)].child[branch];
            if (leaf) {
                if (child >= 0 || setup.decode_tree[static_cast<std::size_t>(node)].symbol >= 0 ||
                    setup.decode_tree.size() >= max_nodes) {
                    return false;
                }
                child = static_cast<std::int32_t>(setup.decode_tree.size());
                setup.decode_tree[static_cast<std::size_t>(node)].child[branch] = child;
                setup.decode_tree.emplace_back();
                setup.decode_tree[static_cast<std::size_t>(child)].symbol =
                    static_cast<std::int32_t>(symbol);
            } else {
                if (setup.decode_tree[static_cast<std::size_t>(node)].symbol >= 0) return false;
                if (child < 0) {
                    if (setup.decode_tree.size() >= max_nodes) return false;
                    child = static_cast<std::int32_t>(setup.decode_tree.size());
                    setup.decode_tree[static_cast<std::size_t>(node)].child[branch] = child;
                    setup.decode_tree.emplace_back();
                }
                node = child;
            }
        }

        auto update_length = static_cast<std::uint32_t>(length);
        while (update_length > 0) {
            if ((markers[update_length] & 1U) != 0) {
                markers[update_length] = markers[update_length - 1U] << 1U;
                break;
            }
            ++markers[update_length];
            --update_length;
        }
        auto previous = code;
        for (std::uint32_t next_length = static_cast<std::uint32_t>(length) + 1U;
             next_length < markers.size(); ++next_length) {
            const auto old_marker = markers[next_length];
            if (previous != (old_marker >> 1U)) break;
            markers[next_length] = markers[next_length - 1U] << 1U;
            previous = old_marker;
        }
    }
    return true;
}

struct UnpackedFloat {
    std::int32_t mantissa{};
    std::int32_t exponent{};
};

UnpackedFloat unpack_float32(std::uint32_t value) noexcept {
    auto mantissa = value & 0x1fffffU;
    auto exponent = static_cast<std::int32_t>((value & 0x7fe00000U) >> 21U) - 788;
    if (mantissa == 0) return {};
    while ((mantissa & 0x40000000U) == 0) {
        mantissa <<= 1U;
        --exponent;
    }
    auto signed_mantissa = static_cast<std::int64_t>(mantissa);
    if ((value & 0x80000000U) != 0) signed_mantissa = -signed_mantissa;
    return {static_cast<std::int32_t>(signed_mantissa), exponent};
}

std::optional<std::int32_t> align_mantissa(std::int64_t mantissa,
                                           std::int32_t shift) noexcept {
    if (shift >= 0) {
        if (mantissa == 0) return 0;
        if (shift > 31) return std::nullopt;
        const auto factor = std::int64_t{1} << shift;
        if (mantissa > std::numeric_limits<std::int32_t>::max() / factor ||
            mantissa < std::numeric_limits<std::int32_t>::min() / factor) {
            return std::nullopt;
        }
        return static_cast<std::int32_t>(mantissa * factor);
    }
    const auto right_shift = -static_cast<std::int64_t>(shift);
    if (right_shift >= 63) return mantissa < 0 ? -1 : 0;
    const auto shifted = mantissa >> right_shift;
    if (shifted < std::numeric_limits<std::int32_t>::min() ||
        shifted > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::int32_t>(shifted);
}

} // namespace

std::optional<VorbisCodebookSetup> parse_vorbis_codebook_setup(
    std::span<const std::uint8_t> setup_bits,
    std::size_t max_entries,
    std::size_t max_lookup_values,
    std::size_t max_decode_nodes) {
    LsbBitReader bits(setup_bits);
    std::uint32_t value = 0;
    if (!bits.read(24, value) || value != 0x564342U) return std::nullopt;

    VorbisCodebookSetup setup;
    if (!bits.read(16, value) || value == 0) return std::nullopt;
    setup.dimensions = static_cast<std::uint16_t>(value);
    if (!bits.read(24, value) || value == 0 || value > max_entries) return std::nullopt;
    setup.entries = value;
    setup.codeword_lengths.resize(setup.entries, 0);

    std::uint32_t ordered = 0;
    if (!bits.read(1, ordered)) return std::nullopt;
    if (ordered != 0) {
        std::uint32_t current_length = 0;
        if (!bits.read(5, current_length)) return std::nullopt;
        ++current_length;
        std::uint32_t current_entry = 0;
        while (current_entry < setup.entries) {
            const auto width = ilog(setup.entries - current_entry);
            std::uint32_t count = 0;
            if (!bits.read(width, count) || count > setup.entries - current_entry ||
                current_length > 32) {
                return std::nullopt;
            }
            std::fill_n(setup.codeword_lengths.begin() + current_entry, count,
                        static_cast<std::uint8_t>(current_length));
            current_entry += count;
            ++current_length;
        }
    } else {
        std::uint32_t sparse = 0;
        if (!bits.read(1, sparse)) return std::nullopt;
        for (auto& length : setup.codeword_lengths) {
            std::uint32_t present = 1;
            if (sparse != 0 && !bits.read(1, present)) return std::nullopt;
            if (present != 0) {
                if (!bits.read(5, value)) return std::nullopt;
                length = static_cast<std::uint8_t>(value + 1U);
            }
        }
    }
    if (!valid_code_lengths(setup.codeword_lengths) ||
        std::none_of(setup.codeword_lengths.begin(), setup.codeword_lengths.end(),
                     [](std::uint8_t length) { return length != 0; })) {
        return std::nullopt;
    }
    if (!build_decode_tree(setup, max_decode_nodes)) return std::nullopt;

    if (!bits.read(4, value) || value > 2) return std::nullopt;
    setup.lookup_type = static_cast<std::uint8_t>(value);
    if (setup.lookup_type != 0) {
        if (!bits.read(32, setup.minimum_value_bits) ||
            !bits.read(32, setup.delta_value_bits) || !bits.read(4, value)) {
            return std::nullopt;
        }
        setup.value_bits = static_cast<std::uint8_t>(value + 1U);
        std::uint32_t sequence = 0;
        if (!bits.read(1, sequence)) return std::nullopt;
        setup.sequence = sequence != 0;

        const auto lookup_count = setup.lookup_type == 1
            ? lookup1_values(setup.entries, setup.dimensions)
            : static_cast<std::uint64_t>(setup.entries) * setup.dimensions;
        if (lookup_count == 0 || lookup_count > max_lookup_values) return std::nullopt;
        setup.lookup_values = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(lookup_count, std::numeric_limits<std::uint32_t>::max()));
        setup.lookup_multiplicands.reserve(static_cast<std::size_t>(lookup_count));
        for (std::uint64_t i = 0; i < lookup_count; ++i) {
            if (!bits.read(setup.value_bits, value)) return std::nullopt;
            setup.lookup_multiplicands.push_back(static_cast<std::uint16_t>(value));
        }
        const auto minimum = unpack_float32(setup.minimum_value_bits);
        const auto delta = unpack_float32(setup.delta_value_bits);
        setup.minimum_mantissa = minimum.mantissa;
        setup.minimum_exponent = minimum.exponent;
        setup.delta_mantissa = delta.mantissa;
        setup.delta_exponent = delta.exponent;
    }

    setup.bits_consumed = bits.bit_offset();
    return setup;
}

std::optional<std::uint32_t> decode_vorbis_codebook_symbol(
    const VorbisCodebookSetup& setup,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset) {
    if (setup.decode_tree.empty() || bit_offset > packet.size() * 8) return std::nullopt;
    std::int32_t node = 0;
    for (std::size_t depth = 0; depth < setup.codeword_lengths.size() + 32U; ++depth) {
        if (setup.decode_tree[static_cast<std::size_t>(node)].symbol >= 0) {
            return static_cast<std::uint32_t>(
                setup.decode_tree[static_cast<std::size_t>(node)].symbol);
        }
        if (bit_offset >= packet.size() * 8) return std::nullopt;
        const auto byte_index = bit_offset / 8;
        const auto bit_index = static_cast<unsigned>(bit_offset % 8);
        const auto branch = static_cast<std::uint8_t>((packet[byte_index] >> bit_index) & 1U);
        ++bit_offset;
        node = setup.decode_tree[static_cast<std::size_t>(node)].child[branch];
        if (node < 0 || static_cast<std::size_t>(node) >= setup.decode_tree.size()) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<std::vector<std::int32_t>> decode_vorbis_codebook_vector(
    const VorbisCodebookSetup& setup,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    std::int32_t target_exponent) {
    if (setup.lookup_type == 0 || setup.lookup_values == 0) return std::nullopt;
    const auto symbol = decode_vorbis_codebook_symbol(setup, packet, bit_offset);
    if (!symbol || *symbol >= setup.entries) return std::nullopt;

    std::vector<std::int32_t> values;
    values.reserve(setup.dimensions);
    auto entry = *symbol;
    std::int64_t sequence_sum = 0;
    for (std::uint16_t component = 0; component < setup.dimensions; ++component) {
        std::uint32_t multiplicand_index = 0;
        if (setup.lookup_type == 1) {
            multiplicand_index = entry % setup.lookup_values;
            entry /= setup.lookup_values;
        } else {
            const auto index = static_cast<std::uint64_t>(*symbol) * setup.dimensions + component;
            if (index >= setup.lookup_multiplicands.size()) return std::nullopt;
            multiplicand_index = static_cast<std::uint32_t>(index);
        }
        if (multiplicand_index >= setup.lookup_multiplicands.size()) return std::nullopt;
        const auto product = static_cast<std::int64_t>(setup.lookup_multiplicands[multiplicand_index]) *
            setup.delta_mantissa;
        const auto delta = align_mantissa(product, setup.delta_exponent - target_exponent);
        const auto minimum = align_mantissa(setup.minimum_mantissa,
                                            setup.minimum_exponent - target_exponent);
        if (!delta || !minimum) return std::nullopt;
        auto decoded = static_cast<std::int64_t>(*delta) + *minimum;
        if (setup.sequence) decoded += sequence_sum;
        if (decoded < std::numeric_limits<std::int32_t>::min() ||
            decoded > std::numeric_limits<std::int32_t>::max()) {
            return std::nullopt;
        }
        values.push_back(static_cast<std::int32_t>(decoded));
        if (setup.sequence) sequence_sum = decoded;
    }
    return values;
}

std::optional<std::int32_t> decode_vorbis_codebook_scalar(
    const VorbisCodebookSetup& setup,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset) {
    const auto symbol = decode_vorbis_codebook_symbol(setup, packet, bit_offset);
    if (!symbol || *symbol >= setup.entries) return std::nullopt;
    if (setup.lookup_type == 0) return static_cast<std::int32_t>(*symbol);
    if (setup.dimensions != 1 || setup.lookup_values == 0) return std::nullopt;

    std::uint64_t multiplicand_index = *symbol;
    if (setup.lookup_type == 1) multiplicand_index %= setup.lookup_values;
    if (multiplicand_index >= setup.lookup_multiplicands.size()) return std::nullopt;
    return static_cast<std::int32_t>(setup.lookup_multiplicands[multiplicand_index]);
}

} // namespace echo::audio
