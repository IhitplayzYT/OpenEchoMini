#include "echo/runtime/event_flags.hpp"

#include <cstddef>

namespace echo::runtime {

EventFlagBank::EventFlagBank(std::span<std::uint32_t> words) noexcept
    : words_(words) {}

bool EventFlagBank::test(std::uint32_t event_id) const noexcept {
    const auto word = static_cast<std::size_t>(event_id >> 5);
    if (word >= words_.size()) return false;
    const auto mask = std::uint32_t{1} << (event_id & 31u);
    return (words_[word] & mask) != 0;
}

bool EventFlagBank::set(std::uint32_t event_id) noexcept {
    const auto word = static_cast<std::size_t>(event_id >> 5);
    if (word >= words_.size()) return false;
    words_[word] |= std::uint32_t{1} << (event_id & 31u);
    return true;
}

bool EventFlagBank::clear(std::uint32_t event_id) noexcept {
    const auto word = static_cast<std::size_t>(event_id >> 5);
    if (word >= words_.size()) return false;
    words_[word] &= ~(std::uint32_t{1} << (event_id & 31u));
    return true;
}

bool EventFlagBank::test_and_clear(std::uint32_t event_id) noexcept {
    const auto word = static_cast<std::size_t>(event_id >> 5);
    if (word >= words_.size()) return false;
    const auto mask = std::uint32_t{1} << (event_id & 31u);
    const bool was_set = (words_[word] & mask) != 0;
    if (was_set) words_[word] &= ~mask;
    return was_set;
}

} // namespace echo::runtime
