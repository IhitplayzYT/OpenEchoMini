#include "echo/audio/id3_picture_frame.hpp"

#include <algorithm>

namespace echo::audio {
namespace {

bool all_zero(std::span<const std::uint8_t> bytes) noexcept {
    return std::all_of(bytes.begin(), bytes.end(), [](std::uint8_t byte) {
        return byte == 0;
    });
}

std::string_view as_text(std::span<const std::uint8_t> bytes) noexcept {
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::optional<std::size_t> description_end(
    std::span<const std::uint8_t> data,
    std::size_t cursor,
    std::uint8_t encoding) noexcept {
    if (encoding == 1 || encoding == 2) {
        // The description is UTF-16 and its terminator must begin on a code
        // unit boundary relative to the description start.
        for (std::size_t i = cursor; i + 1 < data.size(); i += 2) {
            if (data[i] == 0 && data[i + 1] == 0) return i + 2;
        }
        return std::nullopt;
    }

    for (std::size_t i = cursor; i < data.size(); ++i) {
        if (data[i] == 0) return i + 1;
    }
    return std::nullopt;
}

std::optional<Id3PictureFrame> parse_picture_data(
    std::span<const std::uint8_t> frame,
    std::string_view frame_id,
    std::size_t frame_offset,
    bool v22) noexcept {
    if (frame.size() < (v22 ? 5U : 4U)) return std::nullopt;

    const auto encoding = frame[0];
    if (encoding > 3) return std::nullopt;

    std::size_t cursor = 1;
    std::string_view mime;
    if (v22) {
        const auto format = as_text(frame.subspan(cursor, 3));
        cursor += 3;
        if (format == "JPG") mime = "image/jpeg";
        else if (format == "PNG") mime = "image/png";
        else if (format == "BMP") mime = "image/bmp";
        else mime = format;
    } else {
        const auto mime_end = std::find(frame.begin() + static_cast<std::ptrdiff_t>(cursor),
                                        frame.end(), 0);
        if (mime_end == frame.end()) return std::nullopt;
        const auto end = static_cast<std::size_t>(mime_end - frame.begin());
        mime = as_text(frame.subspan(cursor, end - cursor));
        cursor = end + 1;
        if (!mime.starts_with("image/")) return std::nullopt;
    }

    if (cursor >= frame.size()) return std::nullopt;
    const auto picture_type = frame[cursor++];
    const auto description = description_end(frame, cursor, encoding);
    if (!description || *description > frame.size()) return std::nullopt;

    const auto image = frame.subspan(*description);
    const auto signature = resources::find_embedded_image(image);
    return Id3PictureFrame{frame_id, mime, picture_type, image, signature, frame_offset};
}

std::uint32_t read_be24(std::span<const std::uint8_t> bytes) noexcept {
    return (static_cast<std::uint32_t>(bytes[0]) << 16U) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           static_cast<std::uint32_t>(bytes[2]);
}

std::optional<std::uint32_t> read_frame_size(
    std::span<const std::uint8_t> bytes,
    std::uint8_t version) noexcept {
    if (version == 2) return read_be24(bytes);
    if (version == 3) {
        return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
               (static_cast<std::uint32_t>(bytes[1]) << 16U) |
               (static_cast<std::uint32_t>(bytes[2]) << 8U) |
               static_cast<std::uint32_t>(bytes[3]);
    }

    std::uint32_t size = 0;
    for (const auto byte : bytes.first(4)) {
        if ((byte & 0x80U) != 0) return std::nullopt;
        size = (size << 7U) | byte;
    }
    return size;
}

} // namespace

std::optional<Module18Id3ImageMime> module18_find_id3_image_mime(
    std::span<const std::uint8_t> frame_data) noexcept {
    constexpr std::string_view marker = "image/";
    const auto scan_size = std::min<std::size_t>(frame_data.size(), 0x200);
    if (scan_size < marker.size()) return std::nullopt;

    for (std::size_t offset = 0; offset + marker.size() <= scan_size; ++offset) {
        bool matched = true;
        for (std::size_t i = 0; i < marker.size(); ++i) {
            if (frame_data[offset + i] != static_cast<std::uint8_t>(marker[i])) {
                matched = false;
                break;
            }
        }
        if (!matched) continue;

        const auto mime_offset = offset + marker.size();
        if (frame_data.size() - mime_offset >= 4 &&
            frame_data[mime_offset] == 'j' && frame_data[mime_offset + 1] == 'p' &&
            frame_data[mime_offset + 2] == 'e' && frame_data[mime_offset + 3] == 'g') {
            return Module18Id3ImageMime{Module18Id3ImageMimeKind::jpeg, mime_offset};
        }
        if (frame_data.size() - mime_offset >= 3 &&
            frame_data[mime_offset] == 'j' && frame_data[mime_offset + 1] == 'p' &&
            frame_data[mime_offset + 2] == 'g') {
            return Module18Id3ImageMime{Module18Id3ImageMimeKind::jpg, mime_offset};
        }
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<Id3PictureFrame> find_id3_picture_frame(
    std::span<const std::uint8_t> tag_payload,
    std::uint8_t major_version) noexcept {
    if (major_version < 2 || major_version > 4) return std::nullopt;

    const std::size_t header_size = major_version == 2 ? 6U : 10U;
    const std::size_t id_size = major_version == 2 ? 3U : 4U;
    std::size_t offset = 0;

    while (offset <= tag_payload.size() && tag_payload.size() - offset >= header_size) {
        const auto header = tag_payload.subspan(offset, header_size);
        if (all_zero(header.first(id_size))) return std::nullopt;

        const auto size_bytes = header.subspan(id_size, major_version == 2 ? 3U : 4U);
        const auto frame_size = read_frame_size(size_bytes, major_version);
        if (!frame_size) return std::nullopt;
        offset += header_size;
        if (*frame_size > tag_payload.size() - offset) return std::nullopt;

        const auto frame_data = tag_payload.subspan(offset, *frame_size);
        const auto id = as_text(header.first(id_size));
        if ((major_version == 2 && id == "PIC") ||
            (major_version != 2 && id == "APIC")) {
            return parse_picture_data(frame_data, id, offset - header_size,
                                      major_version == 2);
        }

        offset += *frame_size;
    }
    return std::nullopt;
}

std::optional<Id3PictureFrame> find_riff_wave_id3_picture_frame(
    std::span<const std::uint8_t> riff_file) noexcept {
    const auto has_fourcc = [riff_file](std::size_t offset,
                                        std::string_view value) noexcept {
        if (offset > riff_file.size() || riff_file.size() - offset < value.size()) {
            return false;
        }
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (riff_file[offset + i] != static_cast<std::uint8_t>(value[i])) return false;
        }
        return true;
    };
    const auto read_u32_le = [riff_file](std::size_t offset) noexcept {
        return static_cast<std::uint32_t>(riff_file[offset]) |
               (static_cast<std::uint32_t>(riff_file[offset + 1]) << 8U) |
               (static_cast<std::uint32_t>(riff_file[offset + 2]) << 16U) |
               (static_cast<std::uint32_t>(riff_file[offset + 3]) << 24U);
    };

    if (riff_file.size() < 12 || !has_fourcc(0, "RIFF") || !has_fourcc(8, "WAVE")) {
        return std::nullopt;
    }
    const auto riff_size = static_cast<std::size_t>(read_u32_le(4));
    if (riff_size < 4 || riff_size > riff_file.size() - 8) return std::nullopt;
    const auto riff_end = 8 + riff_size;

    // Module 18 advances by exactly the declared chunk length; it does not
    // account for RIFF's optional odd-length padding byte in this path.
    std::size_t cursor = 12;
    while (cursor <= riff_end && riff_end - cursor >= 8) {
        const auto chunk_size = static_cast<std::size_t>(read_u32_le(cursor + 4));
        const auto chunk_data = cursor + 8;
        if (chunk_size > riff_end - chunk_data) return std::nullopt;

        if (has_fourcc(cursor, "id3 ")) {
            const auto tag = riff_file.subspan(chunk_data, chunk_size);
            if (tag.size() < 10 || !has_fourcc(chunk_data, "ID3")) return std::nullopt;
            const auto version = tag[3];
            if (version < 2 || version > 4) return std::nullopt;

            std::uint32_t payload_size = 0;
            for (std::size_t i = 6; i < 10; ++i) {
                if ((tag[i] & 0x80U) != 0) return std::nullopt;
                payload_size = (payload_size << 7U) | tag[i];
            }
            if (payload_size > tag.size() - 10) return std::nullopt;

            auto picture = find_id3_picture_frame(tag.subspan(10, payload_size), version);
            if (!picture) return std::nullopt;
            picture->frame_offset += chunk_data + 10;
            return picture;
        }

        cursor = chunk_data + chunk_size;
    }
    return std::nullopt;
}

} // namespace echo::audio
