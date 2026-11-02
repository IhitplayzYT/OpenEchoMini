#include "echo/audio/vorbis_residue.hpp"

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

std::optional<VorbisResidueSetup> parse_vorbis_residue_setup(
    std::span<const std::uint8_t> setup_bits,
    std::uint16_t codebook_count) {
    LsbBitReader bits(setup_bits);
    std::uint32_t value = 0;
    VorbisResidueSetup setup;

    if (!bits.read(16, value) || value > 2) return std::nullopt;
    setup.type = static_cast<std::uint16_t>(value);
    if (!bits.read(24, setup.begin) || !bits.read(24, setup.end) ||
        setup.end < setup.begin || !bits.read(24, value)) {
        return std::nullopt;
    }
    setup.partition_size = value + 1U;
    if (!bits.read(6, value)) return std::nullopt;
    setup.classifications = static_cast<std::uint8_t>(value + 1U);
    if (!bits.read(8, value) || value >= codebook_count) return std::nullopt;
    setup.classbook = static_cast<std::uint8_t>(value);
    setup.classes.resize(setup.classifications);

    for (auto& residue_class : setup.classes) {
        std::uint32_t low_cascade = 0;
        std::uint32_t high_flag = 0;
        if (!bits.read(3, low_cascade) || !bits.read(1, high_flag)) return std::nullopt;
        std::uint32_t high_cascade = 0;
        if (high_flag != 0 && !bits.read(5, high_cascade)) return std::nullopt;
        residue_class.cascade = static_cast<std::uint8_t>(low_cascade | (high_cascade << 3U));
    }

    for (auto& residue_class : setup.classes) {
        for (std::uint8_t stage = 0; stage < 8; ++stage) {
            if ((residue_class.cascade & (1U << stage)) == 0) continue;
            if (!bits.read(8, value) || value >= codebook_count) return std::nullopt;
            residue_class.stage_books[stage] = static_cast<std::uint8_t>(value);
            setup.stage_count = static_cast<std::uint8_t>(stage + 1U);
        }
    }

    setup.bits_consumed = bits.bit_offset();
    return setup;
}

} // namespace echo::audio
