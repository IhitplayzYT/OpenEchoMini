#include "echo/storage/module38_stream_buffer.hpp"

#include <algorithm>

namespace echo::storage {

Module38Availability Module38StreamBuffer::ensure(
    const std::size_t requested) const noexcept {
    if (requested <= available_ &&
        (cursor_ > available_ || available_ - cursor_ < requested)) {
        return available_ < capacity
            ? Module38Availability::short_buffer
            : Module38Availability::refill_needed;
    }
    return Module38Availability::no_refill;
}

std::size_t Module38StreamBuffer::refill(const Refill& read_more) {
    std::size_t writable = 0;
    std::size_t unread = 0;
    if (cursor_ < capacity + 1) {
        if (cursor_ > 0 && available_ >= cursor_) {
            unread = available_ - cursor_;
            std::move(bytes_.begin() + cursor_, bytes_.begin() + available_, bytes_.begin());
        }
        writable = std::min(cursor_, capacity - unread);
    } else {
        cursor_ = 0;
    }

    const auto reported = read_more
        ? read_more(std::span<std::uint8_t>(bytes_.data() + unread, writable))
        : 0;
    const auto appended = std::min(reported, writable);
    available_ = unread + appended;
    cursor_ = 0;
    return appended;
}

void Module38StreamBuffer::skip(const std::size_t bytes, const Refill& read_more) {
    auto remaining = bytes;
    while (remaining > 0xff) {
        if (available_ > 0xff && available_ - cursor_ < 0x100) {
            (void)refill(read_more);
        }
        cursor_ += 0x100;
        remaining -= 0x100;
    }
    if (remaining > 0) {
        if (ensure(remaining) == Module38Availability::refill_needed) {
            (void)refill(read_more);
        }
        cursor_ += remaining;
    }
}

void Module38StreamBuffer::set_position(const std::size_t cursor,
                                       const std::size_t available) noexcept {
    cursor_ = cursor;
    available_ = available;
}

} // namespace echo::storage
