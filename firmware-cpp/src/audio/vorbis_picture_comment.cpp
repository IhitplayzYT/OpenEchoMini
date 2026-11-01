#include "echo/audio/vorbis_picture_comment.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace echo::audio {
namespace {

bool read_le32(const std::span<const std::uint8_t> bytes,
               std::size_t& cursor,
               std::uint32_t& value) noexcept {
    if (cursor > bytes.size() || bytes.size() - cursor < 4U) return false;
    value = static_cast<std::uint32_t>(bytes[cursor])
        | (static_cast<std::uint32_t>(bytes[cursor + 1U]) << 8U)
        | (static_cast<std::uint32_t>(bytes[cursor + 2U]) << 16U)
        | (static_cast<std::uint32_t>(bytes[cursor + 3U]) << 24U);
    cursor += 4U;
    return true;
}

std::uint32_t peek_le32(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

bool take(const std::span<const std::uint8_t> bytes,
          std::size_t& cursor,
          const std::uint32_t length,
          std::span<const std::uint8_t>& view) noexcept {
    if (cursor > bytes.size() || length > bytes.size() - cursor) return false;
    view = bytes.subspan(cursor, length);
    cursor += length;
    return true;
}

int base64_value(const std::uint8_t value) noexcept {
    if (value >= 'A' && value <= 'Z') return value - 'A';
    if (value >= 'a' && value <= 'z') return value - 'a' + 26;
    if (value >= '0' && value <= '9') return value - '0' + 52;
    if (value == '+') return 62;
    if (value == '/') return 63;
    return -1;
}

} // namespace

std::optional<std::span<const std::uint8_t>> find_vorbis_picture_base64_comment(
    const std::span<const std::uint8_t> comment_payload) noexcept {
    if (comment_payload.size() >= 0xff1cU) return std::nullopt;
    constexpr std::string_view key = "METADATA_BLOCK_PICTURE";
    std::size_t cursor = 0;
    std::uint32_t vendor_length = 0;
    std::uint32_t comment_count = 0;
    std::span<const std::uint8_t> vendor;
    if (!read_le32(comment_payload, cursor, vendor_length)
        || !take(comment_payload, cursor, vendor_length, vendor)
        || !read_le32(comment_payload, cursor, comment_count)) {
        return std::nullopt;
    }
    for (std::uint32_t i = 0; i < comment_count; ++i) {
        std::uint32_t comment_length = 0;
        std::span<const std::uint8_t> comment;
        if (!read_le32(comment_payload, cursor, comment_length)
            || !take(comment_payload, cursor, comment_length, comment)) {
            return std::nullopt;
        }
        const auto equals = std::find(comment.begin(), comment.end(), '=');
        if (equals == comment.end()) continue;
        const auto key_length = static_cast<std::size_t>(equals - comment.begin());
        if (key_length != key.size()) continue;
        bool match = true;
        for (std::size_t j = 0; j < key_length; ++j) {
            if (comment[j] != static_cast<std::uint8_t>(key[j])) {
                match = false;
                break;
            }
        }
        if (match) return comment.subspan(key_length + 1U);
    }
    return std::nullopt;
}

std::optional<std::vector<std::uint8_t>> decode_module18_base64(
    const std::span<const std::uint8_t> encoded) {
    if (encoded.size() >= 0xff1cU) return std::nullopt;
    std::vector<std::uint8_t> compact;
    compact.reserve(encoded.size());
    for (const auto ch : encoded) {
        if (ch == ' ' || ch == '\r' || ch == '\n') continue;
        compact.push_back(ch);
    }
    if (compact.empty() || compact.size() % 4U != 0U) return std::nullopt;

    std::vector<std::uint8_t> decoded;
    decoded.reserve((compact.size() / 4U) * 3U);
    for (std::size_t i = 0; i < compact.size(); i += 4U) {
        const bool last = i + 4U == compact.size();
        const auto a = base64_value(compact[i]);
        const auto b = base64_value(compact[i + 1U]);
        if (a < 0 || b < 0) return std::nullopt;
        const auto c_pad = compact[i + 2U] == '=';
        const auto d_pad = compact[i + 3U] == '=';
        if ((c_pad && !d_pad) || ((c_pad || d_pad) && !last)) {
            return std::nullopt;
        }
        const auto c = c_pad ? 0 : base64_value(compact[i + 2U]);
        const auto d = d_pad ? 0 : base64_value(compact[i + 3U]);
        if (c < 0 || d < 0) return std::nullopt;
        decoded.push_back(static_cast<std::uint8_t>((a << 2) | (b >> 4)));
        if (!c_pad) {
            decoded.push_back(static_cast<std::uint8_t>((b << 4) | (c >> 2)));
        }
        if (!d_pad) {
            decoded.push_back(static_cast<std::uint8_t>((c << 6) | d));
        }
    }
    return decoded;
}

std::optional<std::vector<std::uint8_t>> decode_vorbis_picture_comment(
    const std::span<const std::uint8_t> comment_payload) {
    const auto encoded = find_vorbis_picture_base64_comment(comment_payload);
    if (!encoded) return std::nullopt;
    return decode_module18_base64(*encoded);
}

std::optional<std::vector<std::uint8_t>> decode_ogg_vorbis_picture_comment(
    const std::span<const std::uint8_t> ogg_stream) {
    constexpr std::array<std::uint8_t, 4> capture{'O', 'g', 'g', 'S'};
    constexpr std::array<std::uint8_t, 6> vorbis{'v', 'o', 'r', 'b', 'i', 's'};
    constexpr std::size_t max_comment_packet = 0xff1cU;
    std::vector<std::uint8_t> packet;
    std::optional<std::uint32_t> stream_serial;
    std::size_t page = 0;

    while (page < ogg_stream.size()) {
        if (ogg_stream.size() - page < 27U
            || !std::equal(capture.begin(), capture.end(), ogg_stream.begin() + page)
            || ogg_stream[page + 4U] != 0U) {
            return std::nullopt;
        }
        const auto flags = ogg_stream[page + 5U];
        const auto serial = peek_le32(ogg_stream, page + 14U);
        if (!stream_serial) stream_serial = serial;
        if (serial != *stream_serial) {
            packet.clear();
            page += 27U;
            const auto segments = ogg_stream[page - 1U];
            if (segments > ogg_stream.size() - page) return std::nullopt;
            std::size_t body_size = 0;
            for (std::size_t i = 0; i < segments; ++i) body_size += ogg_stream[page + i];
            page += segments;
            if (body_size > ogg_stream.size() - page) return std::nullopt;
            page += body_size;
            continue;
        }

        const auto segment_count = static_cast<std::size_t>(ogg_stream[page + 26U]);
        if (segment_count > ogg_stream.size() - page - 27U) return std::nullopt;
        const auto lacing = page + 27U;
        auto body = lacing + segment_count;
        std::size_t body_size = 0;
        for (std::size_t i = 0; i < segment_count; ++i) body_size += ogg_stream[lacing + i];
        if (body_size > ogg_stream.size() - body) return std::nullopt;

        bool discard_continued_packet = (flags & 0x01U) != 0U && packet.empty();
        if ((flags & 0x01U) == 0U) packet.clear();
        for (std::size_t i = 0; i < segment_count; ++i) {
            const auto lace = static_cast<std::size_t>(ogg_stream[lacing + i]);
            if (!discard_continued_packet) {
                if (lace > max_comment_packet - packet.size()) {
                    packet.clear();
                    discard_continued_packet = true;
                } else {
                    packet.insert(packet.end(), ogg_stream.begin() + body,
                                  ogg_stream.begin() + body + lace);
                }
            }
            body += lace;
            if (lace < 255U) {
                if (!discard_continued_packet && packet.size() >= 7U
                    && packet[0] == 3U
                    && std::equal(vorbis.begin(), vorbis.end(), packet.begin() + 1)) {
                    const auto decoded = decode_vorbis_picture_comment(
                        std::span<const std::uint8_t>(packet).subspan(7U));
                    if (decoded) return decoded;
                }
                packet.clear();
                discard_continued_packet = false;
            }
        }
        page = body;
    }
    return std::nullopt;
}

} // namespace echo::audio
