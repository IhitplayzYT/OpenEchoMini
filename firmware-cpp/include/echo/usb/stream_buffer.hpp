#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::usb {

struct StreamTransferBlock {
    std::size_t slot{};
    std::span<const std::uint8_t> bytes;
};

// Module 11's observed 0x9000-byte circular buffer and alternating 0x800-byte
// transfer blocks. Callback scheduling, endpoint semantics and backpressure
// policy are not represented; writes stop at available ring capacity.
class StreamBuffer {
public:
    static constexpr std::size_t ring_capacity = 0x9000;
    static constexpr std::size_t transfer_block_size = 0x800;

    [[nodiscard]] std::size_t write(std::span<const std::uint8_t> input) noexcept;
    [[nodiscard]] std::size_t available() const noexcept { return used_; }
    [[nodiscard]] std::size_t free_space() const noexcept { return ring_capacity - used_; }

    [[nodiscard]] std::optional<StreamTransferBlock> next_block() noexcept;

private:
    std::array<std::uint8_t, ring_capacity> ring_{};
    std::array<std::array<std::uint8_t, transfer_block_size>, 2> blocks_{};
    std::size_t read_cursor_{};
    std::size_t write_cursor_{};
    std::size_t used_{};
    std::size_t next_slot_{};
};

} // namespace echo::usb
