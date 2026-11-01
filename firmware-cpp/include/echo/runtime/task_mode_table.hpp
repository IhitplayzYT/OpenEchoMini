#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::runtime {

struct TaskModeEntry {
    std::uint32_t object_address{};
    std::uint32_t routing_value{};
};

// Read-only view of module 0's 19-entry task selector table. Target objects
// are kept opaque: their runtime callback records overlap module-owned data.
class TaskModeTable {
public:
    static constexpr std::size_t entry_count = 19;
    static constexpr std::size_t encoded_size = 0x9c;

    [[nodiscard]] static std::optional<TaskModeTable> parse(
        std::span<const std::uint8_t> bytes) noexcept;

    [[nodiscard]] std::uint8_t selected_index() const noexcept { return selected_index_; }
    [[nodiscard]] std::optional<TaskModeEntry> selected() const noexcept;
    [[nodiscard]] std::optional<TaskModeEntry> entry(std::size_t index) const noexcept;

private:
    std::uint8_t selected_index_{};
    std::array<TaskModeEntry, entry_count> entries_{};
};

} // namespace echo::runtime
