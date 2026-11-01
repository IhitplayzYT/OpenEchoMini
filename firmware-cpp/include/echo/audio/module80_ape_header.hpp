#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

enum class Module80ApeHeaderError {
    none,
    truncated,
    bad_magic,
    unsupported_version,
    too_many_frames,
    size_overflow,
};

struct Module80ApeTagFooter {
    std::uint32_t version{};
    std::uint32_t tag_bytes{};
};

// Parses the 32-byte APE footer accepted by module 80's tag-length probe.
[[nodiscard]] std::optional<Module80ApeTagFooter> parse_module80_ape_tag_footer(
    std::span<const std::uint8_t> footer) noexcept;

struct Module80ApeStreamHeader {
    std::uint16_t version{};
    bool descriptor_layout{};
    std::uint16_t version_high_or_padding{};
    std::uint32_t descriptor_bytes{};
    std::uint32_t header_bytes{};
    std::uint32_t seek_table_bytes{};
    std::uint32_t header_data_bytes{};
    std::uint32_t frame_data_bytes{};
    std::uint32_t frame_data_bytes_high{};
    std::uint32_t terminating_data_bytes{};
    std::array<std::uint8_t, 16> file_md5{};
    std::uint16_t compression_level{};
    std::uint16_t format_flags{};
    std::uint32_t blocks_per_frame{};
    std::uint32_t final_frame_blocks{};
    std::uint32_t total_frames{};
    std::uint16_t bits_per_sample{};
    std::uint16_t channels{};
    std::uint32_t sample_rate{};
    std::optional<std::uint32_t> peak_level;
    std::optional<std::uint32_t> legacy_seek_elements;
    std::size_t header_end{};
    std::size_t first_frame_offset{};
};

struct Module80ApeHeaderResult {
    Module80ApeHeaderError error{Module80ApeHeaderError::none};
    Module80ApeStreamHeader header{};
    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Module80ApeHeaderError::none;
    }
};

struct Module80ApeFrameIndexEntry {
    std::uint32_t aligned_offset{};
    std::uint32_t byte_length{};
    std::uint32_t block_count{};
    std::uint8_t leading_alignment_bytes{};
};

// Reproduces the seek-table-backed frame index construction observed in the
// opener. seek_offsets are file-relative values; element zero is ignored
// because the opener derives frame zero from the parsed header lengths.
[[nodiscard]] std::optional<std::vector<Module80ApeFrameIndexEntry>>
build_module80_ape_frame_index(std::uint32_t file_base,
                               std::uint32_t first_frame_offset,
                               std::span<const std::uint32_t> seek_offsets,
                               std::uint32_t blocks_per_frame,
                               std::uint32_t final_frame_blocks);

// Models module80_ape_open's MAC-prefix, versioned header, and frame-count
// checks. Input begins at "MAC "; first_frame_offset is relative to that
// signature and excludes the separately discovered footer-tag length.
[[nodiscard]] Module80ApeHeaderResult parse_module80_ape_stream_header(
    std::span<const std::uint8_t> bytes) noexcept;

} // namespace echo::audio
