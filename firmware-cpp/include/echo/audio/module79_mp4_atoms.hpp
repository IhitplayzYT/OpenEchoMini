#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct Module79Mp4BoxHeader {
    std::size_t offset{};
    std::uint64_t size{};
    std::uint8_t header_bytes{};
    std::array<char, 4> type{};

    [[nodiscard]] std::uint64_t payload_offset() const noexcept {
        return static_cast<std::uint64_t>(offset) + header_bytes;
    }

    [[nodiscard]] std::uint64_t end_offset() const noexcept {
        return static_cast<std::uint64_t>(offset) + size;
    }
};

// Bounds-checked MP4 box header reader for the big-endian size/tag form used
// by module 79. Size 1 selects the 64-bit large-size header; size 0 extends to
// the supplied parent boundary. It does not parse box payloads or ALAC config.
[[nodiscard]] inline std::optional<Module79Mp4BoxHeader> read_module79_mp4_box(
    std::span<const std::uint8_t> bytes, std::size_t offset,
    std::size_t parent_end) noexcept {
    if (parent_end > bytes.size() || offset > parent_end || parent_end - offset < 8) {
        return std::nullopt;
    }
    const auto read_be32 = [&](std::size_t at) noexcept {
        return (static_cast<std::uint32_t>(bytes[at]) << 24) |
               (static_cast<std::uint32_t>(bytes[at + 1]) << 16) |
               (static_cast<std::uint32_t>(bytes[at + 2]) << 8) |
               static_cast<std::uint32_t>(bytes[at + 3]);
    };

    const std::uint32_t short_size = read_be32(offset);
    Module79Mp4BoxHeader box;
    box.offset = offset;
    for (std::size_t i = 0; i < box.type.size(); ++i) {
        box.type[i] = static_cast<char>(bytes[offset + 4 + i]);
    }

    if (short_size == 1) {
        if (parent_end - offset < 16) {
            return std::nullopt;
        }
        box.header_bytes = 16;
        box.size = (static_cast<std::uint64_t>(read_be32(offset + 8)) << 32) |
                   read_be32(offset + 12);
    } else if (short_size == 0) {
        box.header_bytes = 8;
        box.size = parent_end - offset;
    } else {
        box.header_bytes = 8;
        box.size = short_size;
    }

    if (box.size < box.header_bytes || box.size > parent_end - offset) {
        return std::nullopt;
    }
    return box;
}

} // namespace echo::audio
