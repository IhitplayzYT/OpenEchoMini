#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace echo::storage {

struct Module59TransformInterval {
    std::uint32_t start{};
    std::uint32_t length{};
    std::array<std::uint32_t, 4> record{};
};

struct Module59TransformRange {
    std::uint32_t start{};
    std::uint32_t length{};
    std::array<std::uint32_t, 4> record{};
    bool has_transform{};
};

// Host-side model of module 59's sorted linked list of transform ranges.
// Inserting a range replaces overlaps, then adjacent ranges with identical
// 16-byte records are joined as in the firmware helper.
class Module59TransformMap {
public:
    [[nodiscard]] bool insert(std::uint32_t start,
                              std::uint32_t length,
                              std::array<std::uint32_t, 4> record);

    // Returns the transform interval or the surrounding identity gap that
    // contains offset. If offset lies beyond the final interval, the tail
    // range is returned, matching the firmware's end-of-list branch.
    [[nodiscard]] std::optional<Module59TransformRange> lookup(
        std::uint32_t offset, std::uint32_t total_length) const;

    [[nodiscard]] const std::vector<Module59TransformInterval>& intervals() const noexcept {
        return intervals_;
    }

private:
    std::vector<Module59TransformInterval> intervals_;
};

} // namespace echo::storage
