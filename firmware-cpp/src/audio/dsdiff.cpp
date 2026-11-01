#include "echo/audio/dsdiff.hpp"

#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace echo::audio {
namespace {

constexpr std::uint64_t kRawDsdFrameBytes = 0x24c0;

std::uint32_t read_be32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("truncated DSDIFF 32-bit field");
    }
    return (static_cast<std::uint32_t>(bytes[offset]) << 24)
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8)
        | static_cast<std::uint32_t>(bytes[offset + 3]);
}

std::uint16_t read_be16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2) {
        throw std::runtime_error("truncated DSDIFF 16-bit field");
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[offset]) << 8)
        | static_cast<std::uint16_t>(bytes[offset + 1]));
}

std::uint64_t read_be64(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 8) {
        throw std::runtime_error("truncated DSDIFF 64-bit field");
    }
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i) value = (value << 8) | bytes[offset + i];
    return value;
}

bool has_id(std::span<const std::uint8_t> bytes,
            std::size_t offset,
            std::string_view expected) {
    if (expected.size() != 4 || offset > bytes.size() || bytes.size() - offset < 4) {
        return false;
    }
    for (std::size_t i = 0; i < 4; ++i) {
        if (bytes[offset + i] != static_cast<std::uint8_t>(expected[i])) return false;
    }
    return true;
}

std::string read_id(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("truncated DSDIFF chunk identifier");
    }
    std::string value;
    value.reserve(4);
    for (std::size_t i = 0; i < 4; ++i) value.push_back(static_cast<char>(bytes[offset + i]));
    return value;
}

std::size_t chunk_end(std::uint64_t payload_offset,
                      std::uint64_t payload_size,
                      std::size_t limit) {
    if (payload_offset > limit || payload_size > limit - payload_offset) {
        throw std::runtime_error("DSDIFF chunk extends past its container");
    }
    const auto end = payload_offset + payload_size;
    const auto padded_end = end + (payload_size & 1u);
    if (padded_end > limit) throw std::runtime_error("truncated DSDIFF chunk padding");
    return static_cast<std::size_t>(padded_end);
}

void parse_properties(std::span<const std::uint8_t> bytes,
                      std::size_t begin,
                      std::size_t end,
                      DsdiffInfo& info) {
    if (end - begin < 4 || !has_id(bytes, begin, "SND ")) return;
    auto cursor = begin + 4;
    while (cursor < end) {
        if (end - cursor < 12) throw std::runtime_error("truncated DSDIFF property header");
        const auto id = read_id(bytes, cursor);
        const auto payload_size = read_be64(bytes, cursor + 4);
        const auto payload = cursor + 12;
        const auto next = chunk_end(payload, payload_size, end);

        if (id == "FS  " && payload_size == 4) {
            info.sample_rate = read_be32(bytes, payload);
        } else if (id == "CHNL" && payload_size >= 2) {
            info.channels = read_be16(bytes, payload);
        } else if (id == "CMPR" && payload_size >= 4) {
            info.compression_id = read_id(bytes, payload);
            if (info.compression_id == "DSD ") {
                info.compression = DsdCompression::uncompressed_dsd;
            } else if (info.compression_id == "DST ") {
                info.compression = DsdCompression::dst;
            } else {
                info.compression = DsdCompression::unknown;
            }
        }
        cursor = next;
    }
}

} // namespace

DsdiffInfo DsdiffParser::parse(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 16 || !has_id(bytes, 0, "FRM8") || !has_id(bytes, 12, "DSD ")) {
        throw std::runtime_error("not a supported DSDIFF FRM8/DSD container");
    }

    const auto root_size = read_be64(bytes, 4);
    if (root_size < 4 || root_size > std::numeric_limits<std::size_t>::max() - 12) {
        throw std::runtime_error("invalid DSDIFF root size");
    }
    const auto root_end = static_cast<std::size_t>(12 + root_size);
    if (root_end > bytes.size()) throw std::runtime_error("truncated DSDIFF root container");

    DsdiffInfo info;
    info.container_bytes = root_size + 12;
    auto cursor = std::size_t{16};
    while (cursor < root_end) {
        if (root_end - cursor < 12) throw std::runtime_error("truncated DSDIFF chunk header");
        const auto id = read_id(bytes, cursor);
        const auto payload_size = read_be64(bytes, cursor + 4);
        const auto payload = cursor + 12;
        const auto next = chunk_end(payload, payload_size, root_end);

        if (id == "FVER" && payload_size == 4) {
            info.format_version = read_be32(bytes, payload);
        } else if (id == "PROP") {
            parse_properties(bytes, payload, payload + static_cast<std::size_t>(payload_size), info);
        } else if (id == "DSD ") {
            info.data_offset = payload;
            info.data_bytes = payload_size;
            info.compression = DsdCompression::uncompressed_dsd;
            info.raw_dsd_frame_bytes = kRawDsdFrameBytes;
            info.raw_dsd_frame_count = payload_size / kRawDsdFrameBytes;
        } else if (id == "DST ") {
            info.data_offset = payload;
            info.data_bytes = payload_size;
            info.compression = DsdCompression::dst;
        }
        cursor = next;
    }

    return info;
}

} // namespace echo::audio
