#include "echo/audio/dsf_container.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace echo::audio {
namespace {

bool has_id(std::span<const std::uint8_t> bytes, std::size_t offset,
            char a, char b, char c, char d) noexcept {
    return offset <= bytes.size() && bytes.size() - offset >= 4
        && bytes[offset] == static_cast<std::uint8_t>(a)
        && bytes[offset + 1] == static_cast<std::uint8_t>(b)
        && bytes[offset + 2] == static_cast<std::uint8_t>(c)
        && bytes[offset + 3] == static_cast<std::uint8_t>(d);
}

std::uint32_t read_le32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("truncated DSF 32-bit field");
    }
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint64_t read_le64(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 8) {
        throw std::runtime_error("truncated DSF 64-bit field");
    }
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(bytes[offset + i]) << (i * 8);
    }
    return value;
}

} // namespace

DsfInfo DsfContainerParser::parse(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 92 || !has_id(bytes, 0, 'D', 'S', 'D', ' ')) {
        throw std::runtime_error("not a DSF container");
    }

    const auto dsd_chunk_bytes = read_le64(bytes, 4);
    const auto declared_file_bytes = read_le64(bytes, 12);
    const auto metadata_offset = read_le64(bytes, 20);
    if (dsd_chunk_bytes != 28 || declared_file_bytes < 92
        || declared_file_bytes > bytes.size()) {
        throw std::runtime_error("invalid DSF header sizes");
    }
    if (!has_id(bytes, 28, 'f', 'm', 't', ' ')) {
        throw std::runtime_error("DSF fmt chunk missing at expected position");
    }

    const auto fmt_chunk_bytes = read_le64(bytes, 32);
    if (fmt_chunk_bytes < 52 || fmt_chunk_bytes > declared_file_bytes - 28) {
        throw std::runtime_error("invalid DSF fmt chunk size");
    }
    const auto data_header = 28 + static_cast<std::size_t>(fmt_chunk_bytes);
    if (data_header > declared_file_bytes || declared_file_bytes - data_header < 12
        || !has_id(bytes, data_header, 'd', 'a', 't', 'a')) {
        throw std::runtime_error("DSF data chunk missing or truncated");
    }

    const auto data_chunk_bytes = read_le64(bytes, data_header + 4);
    if (data_chunk_bytes < 12 || data_chunk_bytes > declared_file_bytes - data_header) {
        throw std::runtime_error("invalid DSF data chunk size");
    }

    DsfInfo info;
    info.file_bytes = declared_file_bytes;
    if (metadata_offset != 0) {
        if (metadata_offset >= declared_file_bytes) {
            throw std::runtime_error("DSF metadata offset exceeds declared file size");
        }
        info.metadata_offset = metadata_offset;
    }
    info.format_version = read_le32(bytes, 40);
    info.format_id = read_le32(bytes, 44);
    info.channel_type = read_le32(bytes, 48);
    info.channels = read_le32(bytes, 52);
    info.sample_rate = read_le32(bytes, 56);
    info.bits_per_sample = read_le32(bytes, 60);
    info.samples_per_channel = read_le64(bytes, 64);
    info.block_bytes_per_channel = read_le32(bytes, 72);
    info.data_offset = static_cast<std::uint64_t>(data_header) + 12;
    info.data_bytes = data_chunk_bytes - 12;

    if (info.channels == 0 || info.sample_rate == 0 || info.block_bytes_per_channel == 0) {
        throw std::runtime_error("invalid DSF audio format fields");
    }
    return info;
}

std::vector<std::uint8_t> extract_dsf_channel_bytes(
    std::span<const std::uint8_t> file, const DsfInfo& info,
    std::uint32_t channel_index) {
    if (info.channels == 0 || info.block_bytes_per_channel == 0
        || channel_index >= info.channels) {
        throw std::invalid_argument("invalid DSF channel or block configuration");
    }
    if (info.samples_per_channel > std::numeric_limits<std::uint64_t>::max() - 7) {
        throw std::out_of_range("DSF sample count overflow");
    }
    const auto output_bytes64 = (info.samples_per_channel + 7) / 8;
    if (output_bytes64 > std::numeric_limits<std::size_t>::max()) {
        throw std::out_of_range("DSF channel data exceeds addressable memory");
    }
    if (info.data_offset > file.size() || info.data_bytes > file.size() - info.data_offset) {
        throw std::out_of_range("DSF data extent exceeds supplied bytes");
    }

    std::vector<std::uint8_t> output(static_cast<std::size_t>(output_bytes64));
    if (output.empty()) return output;

    const auto block = static_cast<std::uint64_t>(info.block_bytes_per_channel);
    if (output_bytes64 > std::numeric_limits<std::uint64_t>::max() - (block - 1)) {
        throw std::out_of_range("DSF block count overflow");
    }
    const auto block_count = (output_bytes64 + block - 1) / block;
    if (block > std::numeric_limits<std::uint64_t>::max() / info.channels) {
        throw std::out_of_range("DSF channel-group size overflow");
    }
    const auto group_bytes = block * info.channels;
    if (block_count > std::numeric_limits<std::uint64_t>::max() / group_bytes
        || block_count * group_bytes > info.data_bytes) {
        throw std::runtime_error("DSF data chunk is too short for channel blocks");
    }

    std::uint64_t copied = 0;
    for (std::uint64_t block_index = 0; copied < output_bytes64; ++block_index) {
        const auto source_offset = info.data_offset + block_index * group_bytes
            + static_cast<std::uint64_t>(channel_index) * block;
        const auto count = std::min(block, output_bytes64 - copied);
        if (source_offset > file.size() || count > file.size() - source_offset) {
            throw std::out_of_range("DSF channel block exceeds supplied bytes");
        }
        std::copy_n(file.begin() + static_cast<std::ptrdiff_t>(source_offset),
                    static_cast<std::size_t>(count),
                    output.begin() + static_cast<std::ptrdiff_t>(copied));
        copied += count;
    }
    return output;
}

} // namespace echo::audio
