#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

struct DsfInfo {
    std::uint64_t file_bytes{};
    std::optional<std::uint64_t> metadata_offset;
    std::uint32_t format_version{};
    std::uint32_t format_id{};
    std::uint32_t channel_type{};
    std::uint32_t channels{};
    std::uint32_t sample_rate{};
    std::uint32_t bits_per_sample{};
    std::uint64_t samples_per_channel{};
    std::uint32_t block_bytes_per_channel{};
    std::uint64_t data_offset{};
    std::uint64_t data_bytes{};
};

// Specification-guided DSF metadata parser. It does not demultiplex or decode
// DSD sample blocks and does not parse the optional ID3v2 metadata payload.
class DsfContainerParser {
public:
    [[nodiscard]] static DsfInfo parse(std::span<const std::uint8_t> bytes);
};

// Extracts one raw DSD channel from DSF's channel-block-interleaved data,
// dropping final-block padding. Returned bits retain their on-disk order.
[[nodiscard]] std::vector<std::uint8_t> extract_dsf_channel_bytes(
    std::span<const std::uint8_t> file, const DsfInfo& info,
    std::uint32_t channel_index);

} // namespace echo::audio
