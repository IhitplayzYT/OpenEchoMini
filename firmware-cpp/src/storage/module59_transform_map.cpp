#include "echo/storage/module59_transform_map.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace echo::storage {
namespace {

using Wide = std::uint64_t;

Wide end_of(const Module59TransformInterval& interval) {
    return static_cast<Wide>(interval.start) + interval.length;
}

} // namespace

bool Module59TransformMap::insert(const std::uint32_t start,
                                  const std::uint32_t length,
                                  const std::array<std::uint32_t, 4> record) {
    if (length == 0) return false;
    const Wide new_start = start;
    const Wide new_end = new_start + length;
    constexpr Wide address_space_end = Wide{1} << 32;
    if (new_end > address_space_end) return false;

    std::vector<Module59TransformInterval> next;
    next.reserve(intervals_.size() + 2);
    for (const auto& current : intervals_) {
        const Wide current_start = current.start;
        const Wide current_end = end_of(current);
        if (current_end <= new_start || current_start >= new_end) {
            next.push_back(current);
            continue;
        }
        if (current_start < new_start) {
            next.push_back({current.start,
                static_cast<std::uint32_t>(new_start - current_start), current.record});
        }
        if (current_end > new_end) {
            const auto right_start = static_cast<std::uint32_t>(new_end);
            next.push_back({right_start,
                static_cast<std::uint32_t>(current_end - new_end), current.record});
        }
    }
    next.push_back({start, length, record});
    std::sort(next.begin(), next.end(), [](const auto& a, const auto& b) {
        return a.start < b.start;
    });

    intervals_.clear();
    intervals_.reserve(next.size());
    for (const auto& interval : next) {
        if (!intervals_.empty()) {
            auto& previous = intervals_.back();
            if (end_of(previous) == interval.start && previous.record == interval.record) {
                const auto combined = static_cast<Wide>(previous.length) + interval.length;
                if (combined <= std::numeric_limits<std::uint32_t>::max()) {
                    previous.length = static_cast<std::uint32_t>(combined);
                    continue;
                }
            }
        }
        intervals_.push_back(interval);
    }
    return true;
}

std::optional<Module59TransformRange> Module59TransformMap::lookup(
    const std::uint32_t offset, const std::uint32_t total_length) const {
    if (intervals_.empty()) return Module59TransformRange{0, total_length, {}, false};

    const auto& first = intervals_.front();
    if (offset < first.start) return Module59TransformRange{0, first.start, {}, false};

    for (std::size_t i = 0; i < intervals_.size(); ++i) {
        const auto& interval = intervals_[i];
        const Wide interval_end = end_of(interval);
        if (offset >= interval.start && static_cast<Wide>(offset) < interval_end) {
            return Module59TransformRange{interval.start, interval.length,
                                          interval.record, true};
        }
        if (i + 1 < intervals_.size()) {
            const auto& next = intervals_[i + 1];
            if (offset >= interval_end && offset < next.start) {
                return Module59TransformRange{static_cast<std::uint32_t>(interval_end),
                    static_cast<std::uint32_t>(next.start - interval_end), {}, false};
            }
        }
    }

    const auto tail_start = end_of(intervals_.back());
    const auto tail_length = tail_start <= total_length
        ? static_cast<std::uint32_t>(total_length - tail_start)
        : 0u;
    return Module59TransformRange{static_cast<std::uint32_t>(tail_start), tail_length, {}, false};
}

} // namespace echo::storage
