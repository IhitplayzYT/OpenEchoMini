#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace echo::audio {

inline constexpr std::size_t kVorbisFirstBufferChannelStrideBytes = 0x4000;
inline constexpr std::size_t kVorbisSecondBufferRegionOffsetBytes = 0x8000;
inline constexpr std::size_t kVorbisSecondBufferChannelStrideBytes = 0x2000;
inline constexpr std::size_t kVorbisFirstBufferCapacityWords = 0x1000;
inline constexpr std::size_t kVorbisSecondBufferCapacityWords = 0x0800;

struct VorbisChannelBufferSlot {
    std::size_t first_buffer_offset_bytes{};
    std::size_t first_buffer_capacity_words{};
    std::size_t second_buffer_offset_bytes{};
    std::size_t second_buffer_capacity_words{};
};

// FUN_01004bbc assigns each channel a 0x4000-byte first store and a 0x2000-
// byte second store, with the second store region beginning 0x8000 bytes after
// the first base. Offsets are relative to caller-supplied arena bases.
[[nodiscard]] inline std::optional<std::vector<VorbisChannelBufferSlot>>
plan_vorbis_channel_buffer_slots(std::uint8_t channel_count) {
    if (channel_count == 0) return std::nullopt;
    const auto count = static_cast<std::size_t>(channel_count);
    if (count - 1U > (std::numeric_limits<std::size_t>::max() -
                      kVorbisSecondBufferRegionOffsetBytes) /
                         kVorbisSecondBufferChannelStrideBytes ||
        count - 1U > std::numeric_limits<std::size_t>::max() /
                         kVorbisFirstBufferChannelStrideBytes) {
        return std::nullopt;
    }

    std::vector<VorbisChannelBufferSlot> slots;
    slots.reserve(count);
    for (std::size_t channel = 0; channel < count; ++channel) {
        slots.push_back({
            channel * kVorbisFirstBufferChannelStrideBytes,
            kVorbisFirstBufferCapacityWords,
            kVorbisSecondBufferRegionOffsetBytes +
                channel * kVorbisSecondBufferChannelStrideBytes,
            kVorbisSecondBufferCapacityWords});
    }
    return slots;
}

} // namespace echo::audio
