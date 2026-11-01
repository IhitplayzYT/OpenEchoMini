#pragma once

#include "echo/audio/mpeg_frame_header.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct MpegLayer3GranuleChannel {
    std::uint16_t part23_length{};
    std::uint16_t big_values{};
    std::uint8_t global_gain{};
    std::uint16_t scalefac_compress{};
    bool window_switching{};
    std::uint8_t block_type{};
    bool mixed_block{};
    std::array<std::uint8_t, 3> table_select{};
    std::array<std::uint8_t, 3> subblock_gain{};
    std::uint8_t region0_count{};
    std::uint8_t region1_count{};
    bool preflag{};
    bool scalefac_scale{};
    bool count1table_select{};
};

struct MpegLayer3SideInfo {
    std::uint16_t firmware_error{};
    std::uint16_t main_data_begin{};
    std::uint8_t private_bits{};
    std::array<std::uint8_t, 2> scfsi_masks{};
    std::array<std::array<MpegLayer3GranuleChannel, 2>, 2> granules{};
    std::uint8_t granule_count{};
    std::uint8_t channel_count{};
    std::size_t bits_consumed{};
    std::uint32_t part23_bits_total{};
    bool crc_checked{};
    bool crc_mismatch{};
    bool reservoir_check_performed{};
    bool reservoir_back_pointer_available{true};
};

struct MpegLayer3SideInfoInput {
    std::span<const std::uint8_t> side_info_bits;
    MpegVersion version{MpegVersion::v1};
    std::uint8_t channel_count{2};
    bool crc_present{};
    bool crc_matches{true};
    bool crc_check_suppressed{};
    std::optional<std::uint32_t> available_reservoir_bytes;
};

// Parses the frame's Layer III side-information bitstream. The span starts
// immediately after any frame header/CRC bytes; it does not include main data.
[[nodiscard]] MpegLayer3SideInfo parse_mpeg_layer3_side_info(
    const MpegLayer3SideInfoInput& input) noexcept;

} // namespace echo::audio
