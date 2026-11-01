#include "echo/audio/mpeg_layer3_side_info.hpp"

#include "echo/bitstream/msb_bit_reader.hpp"

namespace echo::audio {
namespace {

constexpr std::uint16_t kSideInfoTooShort = 0x231;
constexpr std::uint16_t kCrcMismatch = 0x201;
constexpr std::uint16_t kTooManyBigValues = 0x232;
constexpr std::uint16_t kZeroSwitchedBlockType = 0x233;
constexpr std::uint16_t kScfsiOnShortBlock = 0x234;
constexpr std::uint16_t kReservoirBackPointerUnavailable = 0x235;

bool read_value(echo::bitstream::MsbBitReader& reader, unsigned width,
                std::uint32_t& value) noexcept {
    const auto result = reader.read(width);
    if (!result) return false;
    value = *result;
    return true;
}

} // namespace

MpegLayer3SideInfo parse_mpeg_layer3_side_info(
    const MpegLayer3SideInfoInput& input) noexcept {
    MpegLayer3SideInfo result{};
    result.channel_count = input.channel_count;
    if (input.channel_count == 0 || input.channel_count > 2) {
        result.firmware_error = kSideInfoTooShort;
        return result;
    }

    result.granule_count = input.version == MpegVersion::v1 ? 2 : 1;
    const bool mpeg1 = input.version == MpegVersion::v1;
    const bool check_crc = input.crc_present && !input.crc_check_suppressed;
    result.crc_checked = check_crc;
    result.crc_mismatch = check_crc && !input.crc_matches;
    if (result.crc_mismatch) result.firmware_error = kCrcMismatch;

    echo::bitstream::MsbBitReader reader(input.side_info_bits);
    std::uint32_t value = 0;
    if (!read_value(reader, mpeg1 ? 9 : 8, value)) {
        result.firmware_error = kSideInfoTooShort;
        result.bits_consumed = reader.bits_consumed();
        return result;
    }
    result.main_data_begin = static_cast<std::uint16_t>(value);

    const unsigned private_width = mpeg1
        ? (input.channel_count == 1 ? 5u : 3u)
        : (input.channel_count == 1 ? 1u : 2u);
    if (!read_value(reader, private_width, value)) {
        result.firmware_error = kSideInfoTooShort;
        result.bits_consumed = reader.bits_consumed();
        return result;
    }
    result.private_bits = static_cast<std::uint8_t>(value);

    if (mpeg1) {
        for (std::uint8_t channel = 0; channel < input.channel_count; ++channel) {
            if (!read_value(reader, 4, value)) {
                result.firmware_error = kSideInfoTooShort;
                result.bits_consumed = reader.bits_consumed();
                return result;
            }
            result.scfsi_masks[channel] = static_cast<std::uint8_t>(value);
        }
    }

    for (std::uint8_t granule = 0; granule < result.granule_count; ++granule) {
        for (std::uint8_t channel = 0; channel < input.channel_count; ++channel) {
            auto& info = result.granules[granule][channel];
            const auto read_into = [&](unsigned width, auto& destination) {
                if (!read_value(reader, width, value)) return false;
                destination = value;
                return true;
            };

            if (!read_into(12, info.part23_length) ||
                !read_into(9, info.big_values) ||
                !read_into(8, info.global_gain) ||
                !read_into(mpeg1 ? 4u : 9u, info.scalefac_compress)) {
                result.firmware_error = kSideInfoTooShort;
                result.bits_consumed = reader.bits_consumed();
                return result;
            }
            result.part23_bits_total += info.part23_length;

            if (!read_into(1, info.window_switching)) {
                result.firmware_error = kSideInfoTooShort;
                result.bits_consumed = reader.bits_consumed();
                return result;
            }
            if (info.window_switching) {
                if (!read_into(2, info.block_type) ||
                    !read_into(1, info.mixed_block)) {
                    result.firmware_error = kSideInfoTooShort;
                    result.bits_consumed = reader.bits_consumed();
                    return result;
                }
                for (unsigned table = 0; table < 2; ++table) {
                    if (!read_into(5, info.table_select[table])) {
                        result.firmware_error = kSideInfoTooShort;
                        result.bits_consumed = reader.bits_consumed();
                        return result;
                    }
                }
                for (auto& gain : info.subblock_gain) {
                    if (!read_into(3, gain)) {
                        result.firmware_error = kSideInfoTooShort;
                        result.bits_consumed = reader.bits_consumed();
                        return result;
                    }
                }
                info.region0_count = info.block_type == 2 && !info.mixed_block ? 8 : 7;
                info.region1_count = 0x24;
                if (info.block_type == 0 && result.firmware_error == 0)
                    result.firmware_error = kZeroSwitchedBlockType;
                if (mpeg1 && info.block_type == 2 &&
                    result.scfsi_masks[channel] != 0 && result.firmware_error == 0)
                    result.firmware_error = kScfsiOnShortBlock;
            } else {
                for (auto& table : info.table_select) {
                    if (!read_into(5, table)) {
                        result.firmware_error = kSideInfoTooShort;
                        result.bits_consumed = reader.bits_consumed();
                        return result;
                    }
                }
                if (!read_into(4, info.region0_count) ||
                    !read_into(3, info.region1_count)) {
                    result.firmware_error = kSideInfoTooShort;
                    result.bits_consumed = reader.bits_consumed();
                    return result;
                }
            }

            if (!read_into(mpeg1 ? 1u : 0u, info.preflag) ||
                !read_into(1, info.scalefac_scale) ||
                !read_into(1, info.count1table_select)) {
                result.firmware_error = kSideInfoTooShort;
                result.bits_consumed = reader.bits_consumed();
                return result;
            }
            if (info.big_values > 0x120 && result.firmware_error == 0)
                result.firmware_error = kTooManyBigValues;
        }
    }

    result.bits_consumed = reader.bits_consumed();
    if (input.available_reservoir_bytes) {
        result.reservoir_check_performed = true;
        result.reservoir_back_pointer_available =
            result.main_data_begin <= *input.available_reservoir_bytes;
        if (!result.reservoir_back_pointer_available && result.firmware_error == 0)
            result.firmware_error = kReservoirBackPointerUnavailable;
    }
    return result;
}

} // namespace echo::audio
