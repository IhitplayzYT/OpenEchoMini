#include "echo/runtime/module0_memory.hpp"

#include <algorithm>
#include <cstring>

namespace echo::runtime {

void fill_module0_bytes(std::span<std::uint8_t> destination,
                        std::uint8_t value) noexcept {
    std::fill(destination.begin(), destination.end(), value);
}

bool move_module0_bytes(std::span<std::uint8_t> memory,
                        const std::size_t destination_offset,
                        const std::size_t source_offset,
                        const std::size_t count) noexcept {
    if (destination_offset > memory.size() || source_offset > memory.size()) {
        return false;
    }
    if (count > memory.size() - destination_offset
        || count > memory.size() - source_offset) {
        return false;
    }
    std::memmove(memory.data() + destination_offset,
                 memory.data() + source_offset, count);
    return true;
}

bool copy_module0_bounded_string(std::span<std::uint8_t> destination,
                                 std::span<const std::uint8_t> source) noexcept {
    if (destination.empty()) return true;

    const auto inspected = std::min(destination.size(), source.size());
    const auto terminator = std::find(source.begin(), source.begin() + inspected, 0);
    const bool found_terminator = terminator != source.begin() + inspected;
    if (!found_terminator && source.size() < destination.size()) return false;

    const auto copied = found_terminator
        ? static_cast<std::size_t>(terminator - source.begin()) + 1
        : destination.size();
    std::copy_n(source.begin(), copied, destination.begin());
    std::fill(destination.begin() + static_cast<std::ptrdiff_t>(copied),
              destination.end(), 0);
    return true;
}

} // namespace echo::runtime
