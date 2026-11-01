#include "echo/audio/module80_ape_header.hpp"

#include <algorithm>
#include <limits>

namespace echo::audio {
namespace {

constexpr std::size_t kApeFooterBytes = 32;
constexpr std::uint32_t kApeMaximumTagVersionExclusive = 2001;
constexpr std::uint32_t kApeMaximumTagBytes = 0x01000020;

bool read_u16(std::span<const std::uint8_t> bytes,
              std::size_t& cursor,
              std::uint16_t& value) noexcept {
    if (cursor > bytes.size() || bytes.size() - cursor < 2) return false;
    value = static_cast<std::uint16_t>(bytes[cursor]) |
            static_cast<std::uint16_t>(bytes[cursor + 1] << 8);
    cursor += 2;
    return true;
}

bool read_u32(std::span<const std::uint8_t> bytes,
              std::size_t& cursor,
              std::uint32_t& value) noexcept {
    if (cursor > bytes.size() || bytes.size() - cursor < 4) return false;
    value = static_cast<std::uint32_t>(bytes[cursor]) |
            (static_cast<std::uint32_t>(bytes[cursor + 1]) << 8) |
            (static_cast<std::uint32_t>(bytes[cursor + 2]) << 16) |
            (static_cast<std::uint32_t>(bytes[cursor + 3]) << 24);
    cursor += 4;
    return true;
}

bool skip(std::span<const std::uint8_t> bytes,
          std::size_t& cursor,
          std::size_t count) noexcept {
    if (cursor > bytes.size() || count > bytes.size() - cursor) return false;
    cursor += count;
    return true;
}

bool add_to(std::size_t& value, std::uint32_t amount) noexcept {
    if (amount > std::numeric_limits<std::size_t>::max() - value) return false;
    value += amount;
    return true;
}

} // namespace

std::optional<Module80ApeTagFooter> parse_module80_ape_tag_footer(
    std::span<const std::uint8_t> footer) noexcept {
    if (footer.size() < kApeFooterBytes) return std::nullopt;
    footer = footer.last(kApeFooterBytes);
    constexpr std::array<std::uint8_t, 8> magic{
        'A', 'P', 'E', 'T', 'A', 'G', 'E', 'X'};
    if (!std::equal(magic.begin(), magic.end(), footer.begin())) {
        return std::nullopt;
    }

    std::size_t cursor = 8;
    Module80ApeTagFooter tag;
    if (!read_u32(footer, cursor, tag.version) ||
        !read_u32(footer, cursor, tag.tag_bytes) ||
        tag.version >= kApeMaximumTagVersionExclusive ||
        tag.tag_bytes < kApeFooterBytes ||
        tag.tag_bytes > kApeMaximumTagBytes) {
        return std::nullopt;
    }
    return tag;
}

std::optional<std::vector<Module80ApeFrameIndexEntry>>
build_module80_ape_frame_index(std::uint32_t file_base,
                               std::uint32_t first_frame_offset,
                               std::span<const std::uint32_t> seek_offsets,
                               std::uint32_t blocks_per_frame,
                               std::uint32_t final_frame_blocks) {
    if (seek_offsets.empty()) return std::vector<Module80ApeFrameIndexEntry>{};
    const auto count = seek_offsets.size();
    if (count > 0xaf0 ||
        first_frame_offset > std::numeric_limits<std::uint32_t>::max() - file_base ||
        final_frame_blocks > std::numeric_limits<std::uint32_t>::max() / 4) {
        return std::nullopt;
    }

    std::vector<std::uint32_t> raw_starts(count);
    raw_starts[0] = file_base + first_frame_offset;
    for (std::size_t i = 1; i < count; ++i) {
        if (seek_offsets[i] > std::numeric_limits<std::uint32_t>::max() - file_base) {
            return std::nullopt;
        }
        raw_starts[i] = file_base + seek_offsets[i];
        if (raw_starts[i] < raw_starts[i - 1]) return std::nullopt;
    }

    std::vector<Module80ApeFrameIndexEntry> entries(count);
    const auto first_raw_start = raw_starts[0];
    for (std::size_t i = 0; i < count; ++i) {
        const auto leading = static_cast<std::uint8_t>(
            (raw_starts[i] - first_raw_start) & 3U);
        const auto aligned = raw_starts[i] - leading;
        const std::uint64_t unaligned_length = i == 0 && count > 1
            ? 0
            : i + 1 == count
                ? static_cast<std::uint64_t>(final_frame_blocks) * 4
                : static_cast<std::uint64_t>(raw_starts[i] - raw_starts[i - 1]);
        const auto padded_length = unaligned_length + leading;
        const auto rounded_length = (padded_length + 3U) & ~std::uint64_t{3};
        if (rounded_length > std::numeric_limits<std::uint32_t>::max()) {
            return std::nullopt;
        }
        entries[i] = {aligned, static_cast<std::uint32_t>(rounded_length),
                      i + 1 == count ? final_frame_blocks : blocks_per_frame,
                      leading};
    }
    return entries;
}

Module80ApeHeaderResult parse_module80_ape_stream_header(
    std::span<const std::uint8_t> bytes) noexcept {
    Module80ApeHeaderResult result;
    auto& h = result.header;
    if (bytes.size() < 4) {
        result.error = Module80ApeHeaderError::truncated;
        return result;
    }
    if (bytes[0] != 'M' || bytes[1] != 'A' || bytes[2] != 'C' || bytes[3] != ' ') {
        result.error = Module80ApeHeaderError::bad_magic;
        return result;
    }

    std::size_t cursor = 4;
    if (!read_u16(bytes, cursor, h.version)) {
        result.error = Module80ApeHeaderError::truncated;
        return result;
    }
    // The signed Thumb compare admits 3950..4170. Its log string says 4120,
    // so preserve the executable comparison as the actual guard.
    if (h.version < 3950 || h.version > 0x104a) {
        result.error = Module80ApeHeaderError::unsupported_version;
        return result;
    }

    if (h.version >= 3980) {
        h.descriptor_layout = true;
        if (!read_u16(bytes, cursor, h.version_high_or_padding) ||
            !read_u32(bytes, cursor, h.descriptor_bytes) ||
            !read_u32(bytes, cursor, h.header_bytes) ||
            !read_u32(bytes, cursor, h.seek_table_bytes) ||
            !read_u32(bytes, cursor, h.header_data_bytes) ||
            !read_u32(bytes, cursor, h.frame_data_bytes) ||
            !read_u32(bytes, cursor, h.frame_data_bytes_high) ||
            !read_u32(bytes, cursor, h.terminating_data_bytes) ||
            bytes.size() - cursor < h.file_md5.size()) {
            result.error = Module80ApeHeaderError::truncated;
            return result;
        }
        for (auto& byte : h.file_md5) byte = bytes[cursor++];
        constexpr std::uint32_t minimum_descriptor_bytes = 0x34;
        if (h.descriptor_bytes > minimum_descriptor_bytes &&
            !skip(bytes, cursor, h.descriptor_bytes - minimum_descriptor_bytes)) {
            result.error = Module80ApeHeaderError::truncated;
            return result;
        }

        if (!read_u16(bytes, cursor, h.compression_level) ||
            !read_u16(bytes, cursor, h.format_flags) ||
            !read_u32(bytes, cursor, h.blocks_per_frame) ||
            !read_u32(bytes, cursor, h.final_frame_blocks) ||
            !read_u32(bytes, cursor, h.total_frames) ||
            !read_u16(bytes, cursor, h.bits_per_sample) ||
            !read_u16(bytes, cursor, h.channels) ||
            !read_u32(bytes, cursor, h.sample_rate)) {
            result.error = Module80ApeHeaderError::truncated;
            return result;
        }
    } else {
        h.descriptor_layout = false;
        h.header_bytes = 0x20;
        if (!read_u16(bytes, cursor, h.compression_level) ||
            !read_u16(bytes, cursor, h.format_flags) ||
            !read_u16(bytes, cursor, h.channels) ||
            !read_u32(bytes, cursor, h.sample_rate) ||
            !read_u32(bytes, cursor, h.header_data_bytes) ||
            !read_u32(bytes, cursor, h.terminating_data_bytes) ||
            !read_u32(bytes, cursor, h.total_frames) ||
            !read_u32(bytes, cursor, h.final_frame_blocks)) {
            result.error = Module80ApeHeaderError::truncated;
            return result;
        }
        if ((h.format_flags & 0x04) != 0) {
            std::uint32_t peak{};
            if (!read_u32(bytes, cursor, peak)) {
                result.error = Module80ApeHeaderError::truncated;
                return result;
            }
            h.peak_level = peak;
            h.header_bytes += 4;
        }
        if ((h.format_flags & 0x10) != 0) {
            std::uint32_t elements{};
            if (!read_u32(bytes, cursor, elements) ||
                elements > std::numeric_limits<std::uint32_t>::max() / 4) {
                result.error = Module80ApeHeaderError::truncated;
                return result;
            }
            h.legacy_seek_elements = elements;
            h.seek_table_bytes = elements * 4;
            h.header_bytes += 4;
        }
        h.bits_per_sample = (h.format_flags & 0x01) != 0 ? 8 :
                            (h.format_flags & 0x08) != 0 ? 24 : 16;
        if (h.version >= 3950) {
            h.blocks_per_frame = 0x48000;
        } else if (h.version >= 3900 ||
                   (h.version >= 3800 && h.sample_rate >= 4000)) {
            h.blocks_per_frame = 0x12000;
        } else {
            h.blocks_per_frame = 0x2400;
        }
    }

    if (h.total_frames > 0xaf0) {
        result.error = Module80ApeHeaderError::too_many_frames;
        return result;
    }

    h.header_end = cursor;
    std::size_t first_frame = 0;
    if (!add_to(first_frame, h.descriptor_layout ? h.descriptor_bytes : 0) ||
        !add_to(first_frame, h.header_bytes) ||
        !add_to(first_frame, h.seek_table_bytes) ||
        !add_to(first_frame, h.header_data_bytes) ||
        !add_to(first_frame, h.terminating_data_bytes)) {
        result.error = Module80ApeHeaderError::size_overflow;
        return result;
    }
    h.first_frame_offset = first_frame;
    return result;
}

} // namespace echo::audio
