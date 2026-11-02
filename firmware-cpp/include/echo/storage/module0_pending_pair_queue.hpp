#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace echo::storage {

// The two words are preserved as opaque values. Module 0 code compares them
// as pairs but the cross-module storage consumer has not exposed their names.
struct Module0PendingPair {
    std::uint32_t first{};
    std::uint32_t second{};

    friend constexpr bool operator==(const Module0PendingPair&,
                                     const Module0PendingPair&) = default;
};

// Interpretation established for the mode-3 file lookup path only. The
// queue remains an opaque pair because other producer modes are not resolved.
struct Module0Mode3DirectoryEntryKey {
    std::uint32_t directory_start_cluster{}; // zero selects the root directory
    std::uint32_t one_based_entry_number{};
};

[[nodiscard]] constexpr Module0Mode3DirectoryEntryKey
module0_mode3_key_from_pending_pair(
    const Module0PendingPair pair) noexcept {
    return {pair.first, pair.second};
}

// FUN_0305be54 decrements the stored one-based entry number before passing it
// to the zero-based directory-record reader; unsigned wrap is intentional.
[[nodiscard]] constexpr std::uint32_t
module0_mode3_zero_based_entry_index(
    const Module0Mode3DirectoryEntryKey key) noexcept {
    return key.one_based_entry_number - 1U;
}

inline constexpr std::size_t kModule0PendingPairCapacity = 500;

// Category 14's lookup rejects an empty table and resets an out-of-range
// requested index to the first entry before reading its opaque pair.
[[nodiscard]] inline std::optional<std::uint16_t>
plan_module0_category14_queue_index(const std::uint32_t requested_index,
                                    const std::uint16_t available_count) noexcept {
    if (available_count == 0U) return std::nullopt;
    if (requested_index >= available_count) return 0U;
    return static_cast<std::uint16_t>(requested_index);
}

struct Module0PendingPairQueue {
    std::array<Module0PendingPair, kModule0PendingPairCapacity> entries{};
    std::uint16_t count{};
};

struct Module0QueueInsertResult {
    bool accepted{};
    bool already_present{};
    bool dropped_oldest{};
    std::uint16_t count{};
};

// Models the explicit-pair path in FUN_03065960. It scans all 500 slots,
// treats a zero pair as the first free slot, and shifts out the oldest entry
// when the list is full. The firmware repairs a zero count to one if it finds
// a duplicate pair in this path.
[[nodiscard]] inline Module0QueueInsertResult module0_queue_insert_explicit(
    Module0PendingPairQueue& queue, const Module0PendingPair pair) noexcept {
    for (const auto& entry : queue.entries) {
        if (entry == pair) {
            if (queue.count == 0) queue.count = 1;
            return {true, true, false, queue.count};
        }
    }

    for (auto& entry : queue.entries) {
        if (entry == Module0PendingPair{}) {
            entry = pair;
            queue.count = static_cast<std::uint16_t>(queue.count + 1U);
            return {true, false, false, queue.count};
        }
    }

    for (std::size_t i = 1; i < queue.entries.size(); ++i) {
        queue.entries[i - 1U] = queue.entries[i];
    }
    queue.entries.back() = pair;
    queue.count = static_cast<std::uint16_t>(kModule0PendingPairCapacity);
    return {true, false, true, queue.count};
}

// Models the configured-pair path in FUN_03065960. It has the same insertion
// and full-list behavior, but a duplicate does not repair a zero count.
[[nodiscard]] inline Module0QueueInsertResult module0_queue_insert_configured(
    Module0PendingPairQueue& queue, const Module0PendingPair pair) noexcept {
    for (const auto& entry : queue.entries) {
        if (entry == pair) return {true, true, false, queue.count};
    }

    for (auto& entry : queue.entries) {
        if (entry == Module0PendingPair{}) {
            entry = pair;
            queue.count = static_cast<std::uint16_t>(queue.count + 1U);
            return {true, false, false, queue.count};
        }
    }

    for (std::size_t i = 1; i < queue.entries.size(); ++i) {
        queue.entries[i - 1U] = queue.entries[i];
    }
    queue.entries.back() = pair;
    queue.count = static_cast<std::uint16_t>(kModule0PendingPairCapacity);
    return {true, false, true, queue.count};
}

// Models FUN_030657b8's matching removal and tail compaction. The firmware
// copies one sentinel pair past the 500 visible entries; this portable model
// clears the vacated last slot because that sentinel's meaning is unknown.
[[nodiscard]] inline bool module0_queue_remove(
    Module0PendingPairQueue& queue, const Module0PendingPair pair) noexcept {
    std::size_t found = queue.entries.size();
    for (std::size_t i = 0; i < queue.entries.size(); ++i) {
        if (queue.entries[i] == pair) {
            found = i;
            break;
        }
    }
    if (found == queue.entries.size()) return false;

    for (std::size_t i = found + 1U; i < queue.entries.size(); ++i) {
        queue.entries[i - 1U] = queue.entries[i];
    }
    queue.entries.back() = {};
    if (queue.count != 0) {
        queue.count = static_cast<std::uint16_t>(queue.count - 1U);
    }
    return true;
}

} // namespace echo::storage
