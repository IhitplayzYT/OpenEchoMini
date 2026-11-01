#include "echo/usb/stream_buffer.hpp"

#include <algorithm>

namespace echo::usb {

std::size_t StreamBuffer::write(const std::span<const std::uint8_t> input) noexcept {
    const auto count = std::min(input.size(), free_space());
    for (std::size_t i = 0; i < count; ++i) {
        ring_[write_cursor_] = input[i];
        write_cursor_ = (write_cursor_ + 1) % ring_capacity;
    }
    used_ += count;
    return count;
}

std::optional<StreamTransferBlock> StreamBuffer::next_block() noexcept {
    if (used_ == 0) {
        return std::nullopt;
    }

    auto& block = blocks_[next_slot_];
    const auto count = std::min(used_, transfer_block_size);
    for (std::size_t i = 0; i < count; ++i) {
        block[i] = ring_[read_cursor_];
        read_cursor_ = (read_cursor_ + 1) % ring_capacity;
    }
    used_ -= count;

    const auto filled_slot = next_slot_;
    next_slot_ ^= 1U;
    return StreamTransferBlock{filled_slot, std::span<const std::uint8_t>(block.data(), count)};
}

} // namespace echo::usb
