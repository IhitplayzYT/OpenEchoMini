#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule31ReadBufferBytes = 0x400;

struct Module31WordReaderState {
    std::array<std::uint8_t, kModule31ReadBufferBytes> buffer{};
    std::uint32_t bytes_available{};
    std::uint32_t cursor{};
    bool eof{};
    bool big_endian{};
    std::array<std::uint8_t, 3> adjacent_refill_flags{};
};

using Module31Refill = std::function<std::size_t(
    std::uint32_t file_handle, std::span<std::uint8_t> destination)>;

// Reads one UTF-16 code unit using FUN_0307be38's 0x400-byte refill buffer.
// Returns false at EOF; a dangling final byte is consumed and yields false.
[[nodiscard]] bool read_module31_utf16_word(
    Module31WordReaderState& state, std::uint32_t file_handle,
    const Module31Refill& refill, std::uint16_t& output);

} // namespace echo::storage
