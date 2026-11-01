#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace echo::audio {

enum class DsdCompression {
    uncompressed_dsd,
    dst,
    unknown,
};

struct DsdiffInfo {
    std::uint64_t container_bytes{};
    std::optional<std::uint32_t> format_version;
    std::optional<std::uint32_t> sample_rate;
    std::optional<std::uint16_t> channels;
    DsdCompression compression{DsdCompression::unknown};
    std::string compression_id;
    std::uint64_t data_offset{};
    std::uint64_t data_bytes{};
    std::uint64_t raw_dsd_frame_bytes{};
    std::uint64_t raw_dsd_frame_count{};
};

class DsdiffParser {
public:
    // Reads DSDIFF container metadata and locates DSD/DST payload chunks.
    // This does not decode DSD or DST audio samples.
    [[nodiscard]] static DsdiffInfo parse(std::span<const std::uint8_t> bytes);
};

} // namespace echo::audio
