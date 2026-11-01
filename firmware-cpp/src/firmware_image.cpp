#include "echo/firmware_image.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace echo::firmware {
namespace {

constexpr std::size_t kMinimumHeaderSize = 0x204;
constexpr std::size_t kVendorOffset = 0x10;
constexpr std::size_t kSdkOffset = 0x30;
constexpr std::size_t kLengthOffset = 0x1f4;
constexpr std::size_t kSignatureOffset = 0x1f8;
constexpr std::size_t kModuleBaseOffset = 0x200;
constexpr std::size_t kModuleCountOffset = 0x204;
constexpr std::size_t kModuleTableOffset = 0x208;
constexpr std::size_t kModuleRecordSize = 8 * sizeof(std::uint32_t);
constexpr std::size_t kHeaderSize = 0x200;

constexpr std::array<std::uint32_t, 3> kRegionHeaderOffsets{
    0x0c8, 0x0f4, 0x148
};

std::uint32_t read_u32_le(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("firmware field extends past end of file");
    }
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::string read_c_string(const std::vector<std::uint8_t>& bytes,
                          std::size_t offset,
                          std::size_t limit) {
    if (offset >= bytes.size()) return {};
    const auto end = std::min(bytes.size(), offset + limit);
    std::string value;
    for (auto i = offset; i < end && bytes[i] != 0; ++i) {
        value.push_back(static_cast<char>(bytes[i]));
    }
    return value;
}

std::optional<std::uint64_t> module_file_offset(std::uint32_t load_address,
                                                std::uint32_t load_base,
                                                std::uint32_t section_size,
                                                std::size_t image_size) {
    if (section_size == 0 || load_address < load_base) return std::nullopt;
    const auto offset = static_cast<std::uint64_t>(load_address - load_base) + 0x200;
    if (offset > image_size || section_size > image_size - offset) return std::nullopt;
    return offset;
}

void write_module_slice(const std::filesystem::path& path,
                        const std::vector<std::uint8_t>& image,
                        std::uint64_t offset,
                        std::uint32_t size) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot create module slice: " + path.string());
    output.write(reinterpret_cast<const char*>(image.data() + offset),
                 static_cast<std::streamsize>(size));
    if (!output) throw std::runtime_error("failed writing module slice: " + path.string());
}

} // namespace

FirmwareImage FirmwareImage::open(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("cannot open firmware image: " + path.string());

    const auto end = static_cast<std::streamoff>(input.tellg());
    if (end < static_cast<std::streamoff>(kMinimumHeaderSize + 4)) {
        throw std::runtime_error("file is too small to contain an RKnano header");
    }
    if (end > static_cast<std::streamoff>(std::numeric_limits<std::uint32_t>::max())) {
        throw std::runtime_error("image exceeds this reader's 32-bit format limit");
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) throw std::runtime_error("failed while reading firmware image");

    return FirmwareImage(path, std::move(bytes));
}

FirmwareImage::FirmwareImage(std::filesystem::path path, std::vector<std::uint8_t> bytes)
    : path_(std::move(path)), bytes_(std::move(bytes)) {
    parse_header();
    if (header_.regions.size() >= 3 && region_is_in_file(header_.regions[2])) {
        resources_ = resources::ResourceCatalog::parse(
            std::span<const std::uint8_t>(bytes_),
            header_.regions[2].file_offset,
            header_.regions[2].size);
    }
}

void FirmwareImage::parse_header() {
    header_.vendor = read_c_string(bytes_, kVendorOffset, 16);
    header_.sdk = read_c_string(bytes_, kSdkOffset, 32);
    header_.signature = read_c_string(bytes_, kSignatureOffset, 8);
    header_.advertised_end = read_u32_le(bytes_, kLengthOffset);
    header_.module_load_base = read_u32_le(bytes_, kModuleBaseOffset);
    header_.module_count = read_u32_le(bytes_, kModuleCountOffset);

    if (kModuleTableOffset > bytes_.size()
        || header_.module_count > (bytes_.size() - kModuleTableOffset) / kModuleRecordSize) {
        throw std::runtime_error("declared module table extends past end of firmware");
    }
    header_.modules.reserve(header_.module_count);
    for (std::size_t i = 0; i < header_.module_count; ++i) {
        const auto offset = kModuleTableOffset + i * kModuleRecordSize;
        ModuleDescriptor module{
            read_u32_le(bytes_, offset + 0),
            read_u32_le(bytes_, offset + 4),
            read_u32_le(bytes_, offset + 8),
            read_u32_le(bytes_, offset + 12),
            read_u32_le(bytes_, offset + 16),
            read_u32_le(bytes_, offset + 20),
            read_u32_le(bytes_, offset + 24),
            read_u32_le(bytes_, offset + 28),
        };
        module.code_file_offset = module_file_offset(
            module.code_load, header_.module_load_base, module.code_size, bytes_.size());
        module.data_file_offset = module_file_offset(
            module.data_load, header_.module_load_base, module.data_size, bytes_.size());
        header_.modules.push_back(module);
    }

    for (const auto header_offset : kRegionHeaderOffsets) {
        const auto offset = static_cast<std::size_t>(header_offset);
        header_.regions.push_back({
            header_offset,
            read_u32_le(bytes_, offset),
            read_u32_le(bytes_, offset + 4),
            read_u32_le(bytes_, offset + 8),
        });
    }

    if (bytes_.size() >= 4) {
        header_.trailer_value = read_u32_le(bytes_, bytes_.size() - 4);
    }

    const auto copy_start = static_cast<std::size_t>(header_.advertised_end);
    if (copy_start <= bytes_.size() && bytes_.size() - copy_start >= kHeaderSize) {
        header_.mirrored_header_matches = true;
        for (std::size_t i = 0; i < kHeaderSize; ++i) {
            if (bytes_[i] != bytes_[copy_start + i]) {
                header_.mirrored_header_matches = false;
                break;
            }
        }
    }
}

bool FirmwareImage::region_is_in_file(const RegionDescriptor& region) const noexcept {
    return region.file_offset <= bytes_.size() && region.end_offset() <= bytes_.size();
}

void FirmwareImage::print_report(std::ostream& output) const {
    output << "File: " << path_.string() << '\n'
           << "Size: " << size() << " bytes (0x" << std::hex << size() << std::dec << ")\n"
           << "Vendor: " << header_.vendor << '\n'
           << "SDK: " << header_.sdk << '\n'
           << "Signature: " << header_.signature << '\n'
           << "Advertised header-copy offset: 0x" << std::hex << header_.advertised_end << std::dec << '\n'
           << "Header copy matches: " << (header_.mirrored_header_matches ? "yes" : "no") << '\n'
           << "Module load base: 0x" << std::hex << header_.module_load_base << std::dec << '\n'
           << "Declared module count: " << header_.module_count
           << ", parsed records: " << header_.modules.size() << '\n'
           << "Final 4-byte value (LE): 0x" << std::hex << header_.trailer_value << std::dec << '\n';

    const auto module_preview = std::min<std::size_t>(8, header_.modules.size());
    for (std::size_t i = 0; i < module_preview; ++i) {
        const auto& module = header_.modules[i];
        output << "  module[" << i << "] code(load=0x" << std::hex << module.code_load
               << ", exec=0x" << module.code_exec << ", size=0x" << module.code_size
               << ", file=";
        if (module.code_file_offset) output << "0x" << *module.code_file_offset;
        else output << "unresolved";
        output << ") data(load=0x" << module.data_load << ", exec=0x" << module.data_exec
               << ", file=";
        if (module.data_file_offset) output << "0x" << *module.data_file_offset;
        else output << "unresolved";
        output << ", size=0x" << module.data_size << ") bss(exec=0x" << module.bss_exec
               << ", size=0x" << module.bss_size << ")" << std::dec << '\n';
    }

    for (std::size_t i = 0; i < header_.regions.size(); ++i) {
        const auto& region = header_.regions[i];
        output << "Region candidate " << i
               << " (descriptor 0x" << std::hex << region.header_offset << "): "
               << "present=" << region.present
               << ", offset=0x" << region.file_offset
               << ", size=0x" << region.size
               << ", end=0x" << region.end_offset()
               << std::dec << ", within file=" << (region_is_in_file(region) ? "yes" : "no") << '\n';
    }

    output << "ROCK26 image table: offset=0x" << std::hex << resources_.rock26_offset
           << std::dec << ", entries=" << resources_.rock26_count << '\n'
           << "Resource metadata: offset=0x" << std::hex << resources_.metadata_offset
           << std::dec << ", parsed=" << resources_.images.size()
           << ", alignment shift=" << resources_.metadata_alignment
           << ", valid image extents=" << resources_.valid_image_count << '\n';
    const auto preview_count = std::min<std::size_t>(8, resources_.images.size());
    for (std::size_t i = 0; i < preview_count; ++i) {
        const auto& image = resources_.images[i];
        output << "  [" << image.metadata_index << "] " << image.name << ' '
               << image.width << 'x' << image.height
               << " data+0x" << std::hex << image.pixel_offset << std::dec
               << (image.extent_is_valid ? " valid" : " invalid/unresolved") << '\n';
    }

    output << "Integrity: not verified by this read-only inspector.\n";
}

std::size_t FirmwareImage::export_bitmaps(const std::filesystem::path& output_directory) const {
    return resources_.export_bitmaps(std::span<const std::uint8_t>(bytes_), output_directory);
}

std::size_t FirmwareImage::apply_bitmaps(
    const std::filesystem::path& input_directory,
    const std::filesystem::path& output_firmware) const {
    const auto source_absolute = std::filesystem::absolute(path_).lexically_normal();
    const auto output_absolute = std::filesystem::absolute(output_firmware).lexically_normal();
    if (source_absolute == output_absolute
        || (std::filesystem::exists(output_firmware)
            && std::filesystem::equivalent(path_, output_firmware))) {
        throw std::runtime_error("output firmware must be a separate file from the source image");
    }
    return resources_.apply_bitmaps(
        std::span<const std::uint8_t>(bytes_), input_directory, output_firmware);
}

std::size_t FirmwareImage::export_modules(const std::filesystem::path& output_directory) const {
    std::filesystem::create_directories(output_directory);
    std::ofstream manifest(output_directory / "modules.tsv", std::ios::trunc);
    if (!manifest) throw std::runtime_error("cannot create module manifest");
    manifest << "index\tkind\texec_address\tfile_offset\tsize\tfile\n";

    std::size_t exported = 0;
    for (std::size_t i = 0; i < header_.modules.size(); ++i) {
        const auto& module = header_.modules[i];
        const auto write_section = [&](std::string_view kind,
                                       std::uint32_t exec_address,
                                       std::uint32_t size,
                                       const std::optional<std::uint64_t>& file_offset) {
            if (size == 0) return;
            if (!file_offset) {
                manifest << i << '\t' << kind << "\t0x" << std::hex << exec_address
                         << "\tINVALID\t0x" << size << "\t\n" << std::dec;
                return;
            }
            std::ostringstream filename;
            filename << "module_" << std::setfill('0') << std::setw(2) << i << '_'
                     << kind << "_" << std::hex << std::setw(8) << exec_address << ".bin";
            write_module_slice(output_directory / filename.str(), bytes_, *file_offset, size);
            manifest << i << '\t' << kind << "\t0x" << std::hex << exec_address
                     << "\t0x" << *file_offset << "\t0x" << size << '\t'
                     << filename.str() << '\n' << std::dec;
            ++exported;
        };
        write_section("code", module.code_exec, module.code_size, module.code_file_offset);
        write_section("data", module.data_exec, module.data_size, module.data_file_offset);
    }
    if (!manifest) throw std::runtime_error("failed writing module manifest");
    return exported;
}

} // namespace echo::firmware
