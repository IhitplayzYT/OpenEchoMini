#include "echo/storage/module31_word_reader.hpp"

#include <algorithm>

namespace echo::storage {
namespace {

bool refill_buffer(Module31WordReaderState& state,
                   const std::uint32_t file_handle,
                   const Module31Refill& refill) {
    if (state.eof || !refill) return false;
    state.bytes_available = static_cast<std::uint32_t>(std::min(
        refill(file_handle, state.buffer), state.buffer.size()));
    state.cursor = 0;
    state.adjacent_refill_flags.fill(0);
    if (state.bytes_available == 0) {
        state.eof = true;
        return false;
    }
    return true;
}

} // namespace

bool read_module31_utf16_word(Module31WordReaderState& state,
                              const std::uint32_t file_handle,
                              const Module31Refill& refill,
                              std::uint16_t& output) {
    if (state.bytes_available <= state.cursor && !refill_buffer(state, file_handle, refill)) {
        return false;
    }
    const auto first = state.buffer[state.cursor++];

    if (state.bytes_available <= state.cursor &&
        !refill_buffer(state, file_handle, refill)) {
        return false;
    }
    const auto second = state.buffer[state.cursor++];

    output = state.big_endian
        ? static_cast<std::uint16_t>((static_cast<std::uint16_t>(first) << 8) | second)
        : static_cast<std::uint16_t>((static_cast<std::uint16_t>(second) << 8) | first);
    return true;
}

} // namespace echo::storage
