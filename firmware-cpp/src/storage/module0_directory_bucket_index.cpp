#include "echo/storage/module0_directory_bucket_index.hpp"

namespace echo::storage {

namespace {

[[nodiscard]] std::uint16_t read_u16_le(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint16_t>(bytes[0]) |
           (static_cast<std::uint16_t>(bytes[1]) << 8U);
}

[[nodiscard]] std::uint32_t read_u32_le(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

} // namespace

std::optional<Module0BrowserWalkFrame> decode_module0_browser_walk_frame(
    const std::span<const std::uint8_t> state, const std::size_t slot) noexcept {
    if (slot >= kModule0BrowserWalkStackSlots) return std::nullopt;
    const std::size_t base = slot * kModule0DirectoryBucketStride;
    constexpr std::size_t required_bytes = kModule0BucketMetadataOffset +
                                           kModule0BucketMetadataBytes;
    if (base > state.size() || state.size() - base < required_bytes) {
        return std::nullopt;
    }

    const auto* frame = state.data() + base;
    Module0BrowserWalkFrame result{};
    result.count_at_08 = read_u16_le(frame + kModule0BucketFileCountOffset);
    result.count_at_0a = read_u16_le(frame + kModule0BucketDirectoryCountOffset);
    result.cursor_at_0c = read_u16_le(frame + kModule0BucketConsumedOffset);
    result.resume_cursor_at_19 = read_u32_le(frame + kModule0BrowserWalkResumeCursorOffset);
    result.key_at_1d = read_u32_le(frame + kModule0BucketPathKeyOffset);
    result.entry_word_at_21 = read_u32_le(frame + kModule0BrowserWalkEntryWordOffset);
    for (std::size_t i = 0; i < result.metadata_at_25.size(); ++i) {
        result.metadata_at_25[i] = frame[kModule0BucketMetadataOffset + i];
    }
    return result;
}

Module0BrowserRootInitialization make_module0_browser_root_initialization(
    const std::uint16_t file_count, const std::uint16_t directory_count,
    const std::uint32_t root_key,
    const std::span<const std::uint8_t, kModule0BucketMetadataBytes> metadata) noexcept {
    Module0BrowserRootInitialization result{};
    result.file_count = file_count;
    result.directory_count = directory_count;
    result.global_cursor = directory_count;
    result.root_key = root_key;
    for (std::size_t i = 0; i < result.metadata.size(); ++i) {
        result.metadata[i] = metadata[i];
    }
    return result;
}

Module0DirectoryBucketIndexResult plan_module0_directory_bucket_index(
    const std::uint32_t initial_index, const std::uint32_t target_key,
    const std::span<const Module0DirectoryBucketSummary> ordered_buckets) noexcept {
    if (target_key == kModule0BrowserRootSentinel) return {0, 0, false};

    std::uint32_t grouped_index = initial_index;
    for (std::size_t i = 0; i < ordered_buckets.size(); ++i) {
        const auto& bucket = ordered_buckets[i];
        if (bucket.lookup_key == target_key) {
            return {grouped_index, i, true};
        }
        grouped_index = static_cast<std::uint16_t>(
            grouped_index + bucket.filtered_file_count);
    }
    return {0, 0, false};
}

Module0DirectoryBucketBacktrackPlan plan_module0_directory_bucket_backtrack(
    const std::uint8_t cursor,
    const std::span<const Module0DirectoryBucketSummary> buckets) noexcept {
    if (cursor != kModule0DirectoryBucketBacktrackCursor) return {};

    for (std::size_t slot = cursor; slot > 0U; --slot) {
        const auto ancestor = slot - 1U;
        if (ancestor >= buckets.size()) continue;

        const auto& bucket = buckets[ancestor];
        if (bucket.consumed_directory_count < bucket.directory_count) {
            return {true, true, static_cast<std::uint8_t>(ancestor),
                    static_cast<std::uint8_t>(ancestor + 1U),
                    static_cast<std::uint16_t>(
                        bucket.consumed_directory_count + 1U)};
        }
    }
    return {true, false, 0, 0, 0};
}

} // namespace echo::storage
