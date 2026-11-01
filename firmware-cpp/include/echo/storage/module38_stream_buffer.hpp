#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace echo::storage {

enum class Module38Availability : std::uint8_t {
    no_refill = 0,
    refill_needed = 1,
    short_buffer = 2,
};

class Module38StreamBuffer {
public:
    static constexpr std::size_t capacity = 0x100;
    using Refill = std::function<std::size_t(std::span<std::uint8_t>)>;

    [[nodiscard]] Module38Availability ensure(std::size_t requested) const noexcept;
    std::size_t refill(const Refill& read_more);
    void skip(std::size_t bytes, const Refill& read_more);

    [[nodiscard]] std::span<const std::uint8_t> buffered_bytes() const noexcept {
        return std::span<const std::uint8_t>(bytes_.data(), available_);
    }
    [[nodiscard]] std::size_t cursor() const noexcept { return cursor_; }
    [[nodiscard]] std::size_t available() const noexcept { return available_; }
    void set_position(std::size_t cursor, std::size_t available) noexcept;

private:
    std::array<std::uint8_t, capacity> bytes_{};
    std::size_t cursor_{};
    std::size_t available_{};
};

} // namespace echo::storage
