#include "echo/runtime/task_mode_table.hpp"

namespace echo::runtime {
namespace {

std::uint32_t read_le32(const std::span<const std::uint8_t> bytes,
                        const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

} // namespace

std::optional<TaskModeTable> TaskModeTable::parse(
    const std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < encoded_size || bytes[0] >= entry_count) {
        return std::nullopt;
    }

    TaskModeTable table;
    table.selected_index_ = bytes[0];
    for (std::size_t i = 0; i < entry_count; ++i) {
        table.entries_[i].object_address = read_le32(bytes, 4 + i * 4);
        table.entries_[i].routing_value = read_le32(bytes, 0x50 + i * 4);
    }
    return table;
}

std::optional<TaskModeEntry> TaskModeTable::selected() const noexcept {
    return entry(selected_index_);
}

std::optional<TaskModeEntry> TaskModeTable::entry(const std::size_t index) const noexcept {
    if (index >= entries_.size()) {
        return std::nullopt;
    }
    return entries_[index];
}

} // namespace echo::runtime
