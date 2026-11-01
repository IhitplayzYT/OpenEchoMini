#pragma once

#include <cstdint>
#include <span>

namespace echo::runtime {

// Module 0 stores event IDs in a word-indexed bitset: word=id>>5,
// bit=id&31. This bounded view models the observed test/set/clear helpers.
class EventFlagBank {
public:
    explicit EventFlagBank(std::span<std::uint32_t> words) noexcept;

    [[nodiscard]] bool test(std::uint32_t event_id) const noexcept;
    [[nodiscard]] bool set(std::uint32_t event_id) noexcept;
    [[nodiscard]] bool clear(std::uint32_t event_id) noexcept;
    // Returns true only when the bit was set, clearing it in that case.
    [[nodiscard]] bool test_and_clear(std::uint32_t event_id) noexcept;

private:
    std::span<std::uint32_t> words_;
};

} // namespace echo::runtime
