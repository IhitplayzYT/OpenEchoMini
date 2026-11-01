#include "echo/resources/resource_catalog.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace echo::resources {
namespace {

constexpr std::size_t kRock26HeaderSize = 32;
constexpr std::size_t kRock26EntrySize = 16;
constexpr std::size_t kMetadataEntrySize = 108;
constexpr std::array<std::uint8_t, 14> kRock26Magic{
    'R','O','C','K','2','6','I','M','A','G','E','R','E','S'
};

std::uint32_t read_u32_le(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("resource field extends past its region");
    }
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint16_t read_u16_le(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2) {
        throw std::runtime_error("bitmap field extends past its file");
    }
    return static_cast<std::uint16_t>(bytes[offset])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

std::vector<std::uint8_t> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("cannot open replacement bitmap: " + path.string());
    const auto end = static_cast<std::streamoff>(input.tellg());
    if (end < 0 || static_cast<std::uint64_t>(end) >
            static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("replacement bitmap has an invalid size: " + path.string());
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) throw std::runtime_error("failed reading replacement bitmap: " + path.string());
    return bytes;
}

std::string read_name(std::span<const std::uint8_t> bytes, std::size_t entry) {
    constexpr std::size_t kNameOffset = 32;
    constexpr std::size_t kNameSize = 64;
    if (entry > bytes.size() || bytes.size() - entry < kNameOffset + kNameSize) return {};
    std::string name;
    for (std::size_t i = 0; i < kNameSize; ++i) {
        const auto c = bytes[entry + kNameOffset + i];
        if (c == 0) break;
        if (c < 0x20 || c > 0x7e) return {};
        name.push_back(static_cast<char>(c));
    }
    return name;
}

bool is_bitmap_name(const std::string& name) {
    constexpr std::string_view suffix = ".BMP";
    return name.size() >= suffix.size()
        && std::equal(suffix.rbegin(), suffix.rend(), name.rbegin());
}

struct Rock26Entry {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t pixel_offset{};
};

bool has_valid_extent(const ImageResource& image, std::size_t region_size) {
    if (image.width == 0 || image.height == 0) return false;
    const auto pixel_count = static_cast<std::uint64_t>(image.width) * image.height;
    if (pixel_count > std::numeric_limits<std::uint32_t>::max() / 2U) return false;
    const auto byte_count = pixel_count * 2;
    return byte_count <= std::numeric_limits<std::uint32_t>::max()
        && image.pixel_offset <= region_size
        && byte_count <= region_size - image.pixel_offset;
}

void write_u16_le(std::ostream& output, std::uint16_t value) {
    output.put(static_cast<char>(value & 0xff));
    output.put(static_cast<char>((value >> 8) & 0xff));
}

void write_u32_le(std::ostream& output, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        output.put(static_cast<char>((value >> shift) & 0xff));
    }
}

std::string safe_filename(const std::string& name, std::size_t index) {
    std::string safe;
    safe.reserve(name.size() + 12);
    for (const auto c : name) {
        const bool accepted = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.'
            || c == '(' || c == ')' || c == ',';
        safe.push_back(accepted ? c : '_');
    }
    std::ostringstream prefix;
    prefix.width(4);
    prefix.fill('0');
    prefix << index << '-';
    return prefix.str() + safe;
}

template <std::size_t N>
std::string bytes_to_hex(const std::array<std::uint8_t, N>& bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(N * 2);
    for (const auto byte : bytes) {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 0x0f]);
    }
    return result;
}

} // namespace

ResourceCatalog ResourceCatalog::parse(std::span<const std::uint8_t> firmware,
                                       std::uint32_t region_file_offset,
                                       std::uint32_t region_size) {
    if (region_file_offset > firmware.size()
        || region_size > firmware.size() - region_file_offset) {
        throw std::runtime_error("image-resource region lies outside the firmware file");
    }
    const auto region = firmware.subspan(region_file_offset, region_size);
    const auto magic = std::search(region.begin(), region.end(),
                                   kRock26Magic.begin(), kRock26Magic.end());
    if (magic == region.end()) throw std::runtime_error("ROCK26IMAGERES table was not found");

    ResourceCatalog result;
    result.region_file_offset = region_file_offset;
    result.region_size = region_size;
    result.rock26_offset = static_cast<std::uint32_t>(magic - region.begin());
    if (region.size() - result.rock26_offset < kRock26HeaderSize) {
        throw std::runtime_error("truncated ROCK26IMAGERES header");
    }
    result.rock26_count = read_u32_le(region, result.rock26_offset + 16);
    const auto entries_start = static_cast<std::size_t>(result.rock26_offset) + kRock26HeaderSize;
    const auto entries_bytes = region.size() - entries_start;
    if (result.rock26_count > entries_bytes / kRock26EntrySize) {
        throw std::runtime_error("ROCK26 entry count exceeds the available region");
    }

    std::vector<Rock26Entry> rock26;
    rock26.reserve(result.rock26_count);
    for (std::size_t i = 0; i < result.rock26_count; ++i) {
        const auto entry = entries_start + i * kRock26EntrySize;
        const auto packed_dimensions = read_u32_le(region, entry);
        rock26.push_back({
            packed_dimensions & 0xffff,
            packed_dimensions >> 16,
            read_u32_le(region, entry + 12),
        });
    }

    // The first ROCK26 pixel offset is an anchor into the parallel metadata table.
    const auto anchor = rock26.empty() ? 0 : rock26.front().pixel_offset;
    const std::array<std::uint8_t, 4> anchor_bytes{
        static_cast<std::uint8_t>(anchor),
        static_cast<std::uint8_t>(anchor >> 8),
        static_cast<std::uint8_t>(anchor >> 16),
        static_cast<std::uint8_t>(anchor >> 24),
    };
    std::size_t table_start = region.size();
    if (!rock26.empty()) {
        auto search = region.begin();
        while (search != region.end()) {
            const auto hit = std::search(search, region.end(), anchor_bytes.begin(), anchor_bytes.end());
            if (hit == region.end()) break;
            const auto hit_offset = static_cast<std::size_t>(hit - region.begin());
            if (hit_offset >= 20) {
                const auto candidate = hit_offset - 20;
                if (candidate <= region.size() - std::min(region.size(), kMetadataEntrySize)
                    && is_bitmap_name(read_name(region, candidate))) {
                    table_start = candidate;
                    break;
                }
            }
            search = hit + 1;
        }
    }
    if (table_start == region.size()) {
        throw std::runtime_error("resource metadata table was not found from its ROCK26 anchor");
    }

    while (table_start >= kMetadataEntrySize) {
        const auto previous = table_start - kMetadataEntrySize;
        if (!is_bitmap_name(read_name(region, previous))) break;
        table_start = previous;
    }
    result.metadata_offset = static_cast<std::uint32_t>(table_start);

    for (std::size_t i = 0; i < result.rock26_count; ++i) {
        const auto entry = table_start + i * kMetadataEntrySize;
        if (entry > region.size() || region.size() - entry < kMetadataEntrySize) break;
        auto name = read_name(region, entry);
        if (name.empty()) break;

        ImageResource image{
            i,
            std::move(name),
            read_u32_le(region, entry + 20),
            read_u32_le(region, entry + 24),
            read_u32_le(region, entry + 28),
            false,
        };
        std::copy_n(region.begin() + static_cast<std::ptrdiff_t>(entry),
                    image.opaque_prefix.size(), image.opaque_prefix.begin());
        std::copy_n(region.begin() + static_cast<std::ptrdiff_t>(entry + 32),
                    image.raw_name_field.size(), image.raw_name_field.begin());
        std::copy_n(region.begin() + static_cast<std::ptrdiff_t>(entry + 96),
                    image.opaque_suffix.size(), image.opaque_suffix.begin());
        image.extent_is_valid = has_valid_extent(image, region.size());
        result.images.push_back(std::move(image));
    }

    // The metadata table can be one record out of step with ROCK26. Keep this
    // derived alignment explicit and repair invalid metadata from the image table.
    int best_shift = 0;
    std::size_t best_votes = 0;
    for (int shift = -3; shift <= 3; ++shift) {
        std::size_t votes = 0;
        const auto sample_count = std::min<std::size_t>(20, rock26.size());
        for (std::size_t resource_index = 0; resource_index < sample_count; ++resource_index) {
            const auto metadata_index = static_cast<std::int64_t>(resource_index) + shift;
            if (metadata_index < 0
                || static_cast<std::uint64_t>(metadata_index) >= result.images.size()) continue;
            if (result.images[static_cast<std::size_t>(metadata_index)].pixel_offset
                == rock26[resource_index].pixel_offset) ++votes;
        }
        if (votes > best_votes) {
            best_votes = votes;
            best_shift = shift;
        }
    }
    result.metadata_alignment = best_shift;

    for (auto& image : result.images) {
        const auto rock26_index = static_cast<std::int64_t>(image.metadata_index) - best_shift;
        if (rock26_index >= 0 && static_cast<std::uint64_t>(rock26_index) < rock26.size()) {
            const auto& source = rock26[static_cast<std::size_t>(rock26_index)];
            if (!image.extent_is_valid) {
                image.pixel_offset = source.pixel_offset;
                image.width = source.width;
                image.height = source.height;
                image.extent_is_valid = has_valid_extent(image, region.size());
            }
        }
        if (image.extent_is_valid) ++result.valid_image_count;
    }
    return result;
}

std::size_t ResourceCatalog::export_bitmaps(
    std::span<const std::uint8_t> firmware,
    const std::filesystem::path& output_directory) const {
    if (region_file_offset > firmware.size()
        || region_size > firmware.size() - region_file_offset) {
        throw std::runtime_error("image-resource region lies outside the firmware file");
    }
    std::filesystem::create_directories(output_directory);
    const auto region = firmware.subspan(region_file_offset, region_size);
    std::ofstream manifest(output_directory / "assets.tsv", std::ios::trunc);
    if (!manifest) throw std::runtime_error("cannot create asset manifest");
    manifest << "metadata_index\trock26_index\tname\tregion_offset\tfile_offset"
                "\twidth\theight\textent_valid\topaque_prefix_hex\traw_name_field_hex"
                "\topaque_suffix_hex\texported_file\n";
    std::size_t exported = 0;

    for (const auto& image : images) {
        const auto rock26_index = static_cast<std::int64_t>(image.metadata_index)
            - metadata_alignment;
        const auto absolute_offset = static_cast<std::uint64_t>(region_file_offset)
            + image.pixel_offset;
        if (!image.extent_is_valid) {
            manifest << image.metadata_index << '\t';
            if (rock26_index >= 0
                && static_cast<std::uint64_t>(rock26_index) < rock26_count) {
                manifest << rock26_index;
            } else {
                manifest << '-';
            }
            manifest << '\t' << image.name << "\t0x" << std::hex
                     << image.pixel_offset << "\t0x" << absolute_offset << std::dec
                     << '\t' << image.width << '\t' << image.height << "\t0\t"
                     << bytes_to_hex(image.opaque_prefix) << '\t'
                     << bytes_to_hex(image.raw_name_field) << '\t'
                     << bytes_to_hex(image.opaque_suffix) << '\t' << '\n';
            continue;
        }
        const auto row_bytes = static_cast<std::uint64_t>(image.width) * 2;
        const auto row_stride = (row_bytes + 3) & ~std::uint64_t{3};
        const auto pixel_bytes = row_stride * image.height;
        if (pixel_bytes > std::numeric_limits<std::uint32_t>::max() - 66) {
            manifest << image.metadata_index << '\t';
            if (rock26_index >= 0
                && static_cast<std::uint64_t>(rock26_index) < rock26_count) {
                manifest << rock26_index;
            } else {
                manifest << '-';
            }
            manifest << '\t' << image.name << "\t0x" << std::hex
                     << image.pixel_offset << "\t0x" << absolute_offset << std::dec
                     << '\t' << image.width << '\t' << image.height << "\t1\t"
                     << bytes_to_hex(image.opaque_prefix) << '\t'
                     << bytes_to_hex(image.raw_name_field) << '\t'
                     << bytes_to_hex(image.opaque_suffix) << "\tSKIPPED_BMP_SIZE\n";
            continue;
        }

        const auto filename = safe_filename(image.name, image.metadata_index);
        const auto path = output_directory / filename;
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("cannot create bitmap: " + path.string());

        // Match the supplied v3.9 reference files: top-down 16-bit BI_BITFIELDS
        // BMP with RGB565 masks. Firmware pixel words are stored high byte first.
        write_u16_le(output, 0x4d42);
        write_u32_le(output, static_cast<std::uint32_t>(66 + pixel_bytes));
        write_u16_le(output, 0);
        write_u16_le(output, 0);
        write_u32_le(output, 66);
        write_u32_le(output, 40);
        write_u32_le(output, image.width);
        write_u32_le(output, static_cast<std::uint32_t>(-
            static_cast<std::int32_t>(image.height)));
        write_u16_le(output, 1);
        write_u16_le(output, 16);
        write_u32_le(output, 3);
        write_u32_le(output, static_cast<std::uint32_t>(pixel_bytes));
        write_u32_le(output, 2835);
        write_u32_le(output, 2835);
        write_u32_le(output, 0);
        write_u32_le(output, 0);
        write_u32_le(output, 0xf800);
        write_u32_le(output, 0x07e0);
        write_u32_le(output, 0x001f);

        const auto row_padding = static_cast<std::size_t>(row_stride - row_bytes);
        const auto pixel_start = static_cast<std::size_t>(image.pixel_offset);
        for (std::uint32_t y = 0; y < image.height; ++y) {
            const auto row = static_cast<std::size_t>(y) * image.width;
            for (std::uint32_t x = 0; x < image.width; ++x) {
                const auto pixel = pixel_start + (row + x) * 2;
                const auto rgb565 = static_cast<std::uint16_t>(
                    (static_cast<std::uint16_t>(region[pixel]) << 8) | region[pixel + 1]);
                write_u16_le(output, rgb565);
            }
            for (std::size_t i = 0; i < row_padding; ++i) output.put('\0');
        }
        if (!output) throw std::runtime_error("failed while writing bitmap: " + path.string());
        manifest << image.metadata_index << '\t';
        if (rock26_index >= 0
            && static_cast<std::uint64_t>(rock26_index) < rock26_count) {
            manifest << rock26_index;
        } else {
            manifest << '-';
        }
        manifest << '\t' << image.name << "\t0x" << std::hex
                 << image.pixel_offset << "\t0x" << absolute_offset << std::dec
                 << '\t' << image.width << '\t' << image.height << "\t1\t"
                 << bytes_to_hex(image.opaque_prefix) << '\t'
                 << bytes_to_hex(image.raw_name_field) << '\t'
                 << bytes_to_hex(image.opaque_suffix) << '\t' << filename << '\n';
        ++exported;
    }
    if (!manifest) throw std::runtime_error("failed while writing asset manifest");
    return exported;
}

std::size_t ResourceCatalog::apply_bitmaps(
    const std::span<const std::uint8_t> firmware,
    const std::filesystem::path& input_directory,
    const std::filesystem::path& output_firmware) const {
    if (region_file_offset > firmware.size()
        || region_size > firmware.size() - region_file_offset) {
        throw std::runtime_error("image-resource region lies outside the firmware file");
    }
    if (!std::filesystem::is_directory(input_directory)) {
        throw std::runtime_error("replacement bitmap directory does not exist: "
                                 + input_directory.string());
    }

    std::vector<std::uint8_t> patched(firmware.begin(), firmware.end());
    std::size_t applied = 0;
    for (const auto& image : images) {
        if (!image.extent_is_valid) continue;
        const auto path = input_directory / safe_filename(image.name, image.metadata_index);
        if (!std::filesystem::exists(path)) continue;
        const auto bmp = read_binary_file(path);
        const auto bytes = std::span<const std::uint8_t>(bmp);

        // The exporter emits a 40-byte BITMAPINFOHEADER followed by three
        // RGB565 masks. Restrict imports to this stable interchange form.
        if (bytes.size() < 66 || bytes[0] != 'B' || bytes[1] != 'M'
            || read_u32_le(bytes, 2) != bytes.size()
            || read_u32_le(bytes, 14) != 40
            || read_u32_le(bytes, 10) < 66
            || read_u16_le(bytes, 26) != 1
            || read_u16_le(bytes, 28) != 16
            || read_u32_le(bytes, 30) != 3
            || read_u32_le(bytes, 54) != 0xf800
            || read_u32_le(bytes, 58) != 0x07e0
            || read_u32_le(bytes, 62) != 0x001f) {
            throw std::runtime_error("replacement is not a supported RGB565 BMP: " + path.string());
        }

        const auto width = std::bit_cast<std::int32_t>(read_u32_le(bytes, 18));
        const auto signed_height = std::bit_cast<std::int32_t>(read_u32_le(bytes, 22));
        const auto height = signed_height < 0
            ? -static_cast<std::int64_t>(signed_height)
            : static_cast<std::int64_t>(signed_height);
        if (width <= 0 || static_cast<std::uint32_t>(width) != image.width
            || height <= 0 || static_cast<std::uint64_t>(height) != image.height) {
            throw std::runtime_error("replacement dimensions must remain "
                + std::to_string(image.width) + "x" + std::to_string(image.height)
                + ": " + path.string());
        }

        const auto pixel_offset = static_cast<std::size_t>(read_u32_le(bytes, 10));
        const auto row_bytes = static_cast<std::uint64_t>(image.width) * 2U;
        const auto row_stride = (row_bytes + 3U) & ~std::uint64_t{3};
        const auto pixel_bytes = row_stride * image.height;
        if (pixel_offset > bytes.size() || pixel_bytes > bytes.size() - pixel_offset) {
            throw std::runtime_error("replacement BMP pixels are truncated: " + path.string());
        }

        const auto region_pixel_offset = static_cast<std::size_t>(region_file_offset)
            + image.pixel_offset;
        for (std::uint32_t y = 0; y < image.height; ++y) {
            const auto source_y = signed_height < 0 ? y : image.height - 1U - y;
            const auto source_row = pixel_offset + static_cast<std::size_t>(source_y * row_stride);
            const auto target_row = region_pixel_offset
                + static_cast<std::size_t>(y) * image.width * 2U;
            for (std::uint32_t x = 0; x < image.width; ++x) {
                const auto source = source_row + static_cast<std::size_t>(x) * 2U;
                const auto target = target_row + static_cast<std::size_t>(x) * 2U;
                patched[target] = bytes[source + 1];
                patched[target + 1] = bytes[source];
            }
        }
        ++applied;
    }
    if (applied == 0) {
        throw std::runtime_error("replacement directory contains no recognized valid image files");
    }

    std::ofstream output(output_firmware, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot create output firmware: " + output_firmware.string());
    output.write(reinterpret_cast<const char*>(patched.data()),
                 static_cast<std::streamsize>(patched.size()));
    if (!output) throw std::runtime_error("failed writing output firmware: " + output_firmware.string());
    return applied;
}

} // namespace echo::resources
