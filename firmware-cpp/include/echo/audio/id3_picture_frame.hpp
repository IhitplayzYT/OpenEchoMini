#pragma once

#include "echo/resources/embedded_image.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace echo::audio {

struct Id3PictureFrame {
    std::string_view frame_id;
    std::string_view mime_type;
    std::uint8_t picture_type{};
    std::span<const std::uint8_t> image_data;
    std::optional<resources::EmbeddedImageSignature> signature;
    std::size_t frame_offset{};
};

enum class Module18Id3ImageMimeKind {
    jpeg,
    jpg,
};

struct Module18Id3ImageMime {
    Module18Id3ImageMimeKind kind{};
    std::size_t mime_offset{};
};

// The module 18 v2.3/v2.4 handlers scan at most the first 512 bytes of a
// frame for `image/` and accept only `jpeg` or `jpg` immediately after it.
[[nodiscard]] std::optional<Module18Id3ImageMime> module18_find_id3_image_mime(
    std::span<const std::uint8_t> frame_data) noexcept;

// Scans an ID3 tag payload (the bytes after the 10-byte ID3 header) for the
// first APIC frame (v2.3/v2.4) or PIC frame (v2.2). Returns bounded spans into
// the caller's input; it does not allocate or decode the image.
[[nodiscard]] std::optional<Id3PictureFrame> find_id3_picture_frame(
    std::span<const std::uint8_t> tag_payload,
    std::uint8_t major_version) noexcept;

// Searches a RIFF/WAVE file for the firmware's lowercase `id3 ` chunk, then
// parses the contained ID3v2 tag and returns its first APIC/PIC image frame.
// Offsets in the result are relative to the complete RIFF/WAVE input.
[[nodiscard]] std::optional<Id3PictureFrame> find_riff_wave_id3_picture_frame(
    std::span<const std::uint8_t> riff_file) noexcept;

} // namespace echo::audio
