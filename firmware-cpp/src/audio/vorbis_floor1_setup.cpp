#include "echo/audio/vorbis_floor1.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>

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

} // namespace

std::optional<VorbisFloor1Setup> parse_vorbis_floor1_setup(
    std::span<const std::uint8_t> setup_bits,
    std::uint16_t codebook_count) {
    static constexpr std::array<std::uint16_t, 4> y_ranges{256, 128, 86, 64};
    LsbBitReader bits(setup_bits);
    std::uint32_t value = 0;
    if (!bits.read(5, value)) return std::nullopt;
    const auto partition_count = value;

    VorbisFloor1Setup setup;
    setup.partition_classes.reserve(partition_count);
    std::uint8_t highest_class = 0;
    for (std::uint32_t i = 0; i < partition_count; ++i) {
        if (!bits.read(4, value)) return std::nullopt;
        setup.partition_classes.push_back(static_cast<std::uint8_t>(value));
        highest_class = std::max(highest_class, static_cast<std::uint8_t>(value));
    }
    const auto class_count = partition_count == 0
        ? 0U
        : static_cast<std::uint32_t>(highest_class) + 1U;
    setup.classes.reserve(class_count);

    for (std::uint32_t i = 0; i < class_count; ++i) {
        VorbisFloor1Class cls;
        if (!bits.read(3, value)) return std::nullopt;
        cls.dimensions = static_cast<std::uint8_t>(value + 1U);
        if (!bits.read(2, value)) return std::nullopt;
        cls.subclass_bits = static_cast<std::uint8_t>(value);

        if (cls.subclass_bits != 0) {
            if (!bits.read(8, value) || value >= codebook_count) return std::nullopt;
            cls.masterbook = static_cast<std::uint8_t>(value);
        }
        const auto subclass_count = 1U << cls.subclass_bits;
        cls.subclass_books.reserve(subclass_count);
        for (std::uint32_t book = 0; book < subclass_count; ++book) {
            if (!bits.read(8, value)) return std::nullopt;
            if (value == 0) {
                cls.subclass_books.push_back(0xff);
            } else {
                const auto index = value - 1U;
                if (index >= codebook_count || index >= 0xffU) return std::nullopt;
                cls.subclass_books.push_back(static_cast<std::uint8_t>(index));
            }
        }
        setup.classes.push_back(std::move(cls));
    }

    if (!bits.read(2, value)) return std::nullopt;
    setup.multiplier = static_cast<std::uint8_t>(value + 1U);
    setup.y_range = y_ranges[setup.multiplier - 1U];
    if (!bits.read(4, value)) return std::nullopt;
    setup.range_bits = static_cast<std::uint8_t>(value);
    const auto endpoint = static_cast<std::uint16_t>(1U << setup.range_bits);

    std::size_t post_count = 2;
    for (const auto class_index : setup.partition_classes) {
        post_count += setup.classes[class_index].dimensions;
        if (post_count > 256) return std::nullopt;
    }
    setup.x_list.reserve(post_count);
    setup.x_list.push_back(0);
    setup.x_list.push_back(endpoint);
    for (const auto class_index : setup.partition_classes) {
        const auto dimensions = setup.classes[class_index].dimensions;
        for (std::uint8_t d = 0; d < dimensions; ++d) {
            if (!bits.read(setup.range_bits, value) || value > endpoint) return std::nullopt;
            setup.x_list.push_back(static_cast<std::uint16_t>(value));
        }
    }

    setup.sorted_post_order.resize(post_count);
    std::iota(setup.sorted_post_order.begin(), setup.sorted_post_order.end(), std::uint8_t{0});
    std::stable_sort(setup.sorted_post_order.begin(), setup.sorted_post_order.end(),
        [&setup](std::uint8_t left, std::uint8_t right) {
            return setup.x_list[left] < setup.x_list[right];
        });

    setup.low_neighbor.assign(post_count, 0);
    setup.high_neighbor.assign(post_count, 1);
    for (std::size_t post = 2; post < post_count; ++post) {
        auto low = std::size_t{0};
        auto high = std::size_t{1};
        for (std::size_t previous = 0; previous < post; ++previous) {
            if (setup.x_list[previous] < setup.x_list[post] &&
                setup.x_list[previous] > setup.x_list[low]) {
                low = previous;
            }
            if (setup.x_list[previous] > setup.x_list[post] &&
                setup.x_list[previous] < setup.x_list[high]) {
                high = previous;
            }
        }
        setup.low_neighbor[post] = static_cast<std::uint8_t>(low);
        setup.high_neighbor[post] = static_cast<std::uint8_t>(high);
    }
    setup.bits_consumed = bits.bit_offset();
    return setup;
}

} // namespace echo::audio
