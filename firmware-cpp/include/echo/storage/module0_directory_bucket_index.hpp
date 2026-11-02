#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

inline constexpr std::uint32_t kModule0BrowserRootSentinel = 0x0ffffff7U;
inline constexpr std::uint8_t kModule0DirectoryBucketBacktrackCursor = 7;
inline constexpr std::size_t kModule0DirectoryBucketStride = 0x47U;
inline constexpr std::size_t kModule0BucketFileCountOffset = 0x08U;
inline constexpr std::size_t kModule0BucketDirectoryCountOffset = 0x0aU;
inline constexpr std::size_t kModule0BucketConsumedOffset = 0x0cU;
inline constexpr std::size_t kModule0BucketPathKeyOffset = 0x1dU;
inline constexpr std::size_t kModule0BucketMetadataOffset = 0x25U;
inline constexpr std::size_t kModule0BucketMetadataBytes = 0x2aU;
inline constexpr std::size_t kModule0BrowserWalkStackSlots = 8U;
inline constexpr std::size_t kModule0BrowserWalkResumeCursorOffset = 0x19U;
inline constexpr std::size_t kModule0BrowserWalkEntryWordOffset = 0x21U;

// Raw frame fields written by FUN_0305a906. Names stay structural where the
// caller-facing meaning has not been established. Each slot occupies 0x47
// bytes in the shared traversal state.
struct Module0BrowserWalkFrame {
    std::uint16_t count_at_08{};
    std::uint16_t count_at_0a{};
    std::uint16_t cursor_at_0c{};
    std::uint32_t resume_cursor_at_19{};
    std::uint32_t key_at_1d{};
    std::uint32_t entry_word_at_21{};
    std::array<std::uint8_t, kModule0BucketMetadataBytes> metadata_at_25{};
};

// Reads one frame from the shared traversal-state byte block. The block must
// include every byte through the selected frame's metadata at +0x25.
[[nodiscard]] std::optional<Module0BrowserWalkFrame>
decode_module0_browser_walk_frame(std::span<const std::uint8_t> state,
                                  std::size_t slot) noexcept;

struct Module0BrowserRootInitialization {
    std::uint8_t depth{};
    std::uint16_t file_count{};
    std::uint16_t directory_count{};
    // FUN_0305aa74 seeds the shared scan cursor from the root directory count.
    std::uint32_t global_cursor{};
    std::uint32_t saved_scan_cursor{};
    std::uint32_t entry_word{};
    std::uint32_t root_key{};
    std::array<std::uint8_t, kModule0BucketMetadataBytes> metadata{};
};

// Models fields explicitly initialized by FUN_0305aa74. The per-frame
// consumed-directory halfword is omitted because this routine does not write
// it, so its initial value depends on surrounding state initialization.
[[nodiscard]] Module0BrowserRootInitialization
make_module0_browser_root_initialization(
    std::uint16_t file_count, std::uint16_t directory_count,
    std::uint32_t root_key,
    std::span<const std::uint8_t, kModule0BucketMetadataBytes> metadata) noexcept;

struct Module0DirectoryBucketSummary {
    // The key's application meaning remains unknown. The counts are the
    // halfwords recovered at bucket offsets +8 and +10.
    std::uint32_t lookup_key{};
    std::uint16_t filtered_file_count{};
    std::uint16_t directory_count{};
    std::uint16_t consumed_directory_count{};
};

struct Module0DirectoryBucketIndexResult {
    // Matches FUN_0305c10a's 16-bit wrapped accumulation after the first add.
    std::uint32_t grouped_index{};
    std::size_t buckets_before_match{};
    bool target_found{};
};

// Computes the count prefix for an already ordered list of materialized
// buckets. Bucket creation/advancement in FUN_0305a906 remains separate.
[[nodiscard]] Module0DirectoryBucketIndexResult
plan_module0_directory_bucket_index(
    std::uint32_t initial_index, std::uint32_t target_key,
    std::span<const Module0DirectoryBucketSummary> ordered_buckets) noexcept;

struct Module0DirectoryBucketBacktrackPlan {
    bool backtrack_branch{};
    bool pending_directory_found{};
    std::uint8_t ancestor_slot{};
    std::uint8_t next_slot{};
    std::uint16_t updated_consumed_count{};
};

// Models FUN_0305a906's cursor-7 search through earlier buckets for an
// unconsumed directory. The caller still performs the directory scan and
// materializes the following bucket.
[[nodiscard]] Module0DirectoryBucketBacktrackPlan
plan_module0_directory_bucket_backtrack(
    std::uint8_t cursor,
    std::span<const Module0DirectoryBucketSummary> buckets) noexcept;

} // namespace echo::storage
