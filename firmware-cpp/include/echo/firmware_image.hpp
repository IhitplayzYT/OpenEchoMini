#pragma once

#include "echo/resources/resource_catalog.hpp"

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace echo::firmware {

struct RegionDescriptor {
    std::uint32_t header_offset{};
    std::uint32_t present{};
    std::uint32_t file_offset{};
    std::uint32_t size{};

    [[nodiscard]] std::uint64_t end_offset() const noexcept {
        return static_cast<std::uint64_t>(file_offset) + size;
    }
};

// Eight-word RKnano module records. Field names follow the public
// RKnano-format reference; addresses are preserved as recorded because their
// mapping to byte ranges in this particular image is not yet established.
struct ModuleDescriptor {
    std::uint32_t code_load{};
    std::uint32_t code_exec{};
    std::uint32_t code_size{};
    std::uint32_t data_load{};
    std::uint32_t data_exec{};
    std::uint32_t data_size{};
    std::uint32_t bss_exec{};
    std::uint32_t bss_size{};
    std::optional<std::uint64_t> code_file_offset;
    std::optional<std::uint64_t> data_file_offset;
};

struct FirmwareHeader {
    std::string vendor;
    std::string sdk;
    std::string signature;
    std::uint32_t advertised_end{};
    std::uint32_t module_load_base{};
    std::uint32_t module_count{};
    std::uint32_t trailer_value{};
    bool mirrored_header_matches{};
    std::vector<ModuleDescriptor> modules;
    std::vector<RegionDescriptor> regions;
};

class FirmwareImage {
public:
    static FirmwareImage open(const std::filesystem::path& path);

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] std::uint64_t size() const noexcept { return bytes_.size(); }
    [[nodiscard]] const FirmwareHeader& header() const noexcept { return header_; }
    [[nodiscard]] const resources::ResourceCatalog& resources() const noexcept { return resources_; }
    [[nodiscard]] bool region_is_in_file(const RegionDescriptor& region) const noexcept;

    void print_report(std::ostream& output) const;
    std::size_t export_bitmaps(const std::filesystem::path& output_directory) const;
    std::size_t apply_bitmaps(const std::filesystem::path& input_directory,
                              const std::filesystem::path& output_firmware) const;
    std::size_t export_modules(const std::filesystem::path& output_directory) const;

private:
    FirmwareImage(std::filesystem::path path, std::vector<std::uint8_t> bytes);
    void parse_header();

    std::filesystem::path path_;
    std::vector<std::uint8_t> bytes_;
    FirmwareHeader header_;
    resources::ResourceCatalog resources_;
};

} // namespace echo::firmware
