#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::image {

inline constexpr std::size_t module19_input_chunk_bytes = 0x1000;
inline constexpr std::size_t module19_eoi_padding_bytes = 0x80;

// A short module 19 read is followed by 64 little-endian 0xd9ff halfwords
// (the JPEG byte sequence ff d9). The caller must provide the padding slack.
[[nodiscard]] bool module19_pad_short_read_with_eoi(
    std::span<std::uint8_t> buffer_with_slack,
    std::size_t bytes_read) noexcept;

} // namespace echo::image
