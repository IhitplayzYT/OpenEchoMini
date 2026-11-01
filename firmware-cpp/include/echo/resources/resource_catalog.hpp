#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace echo::resources {

struct ImageResource {
    std::size_t metadata_index{};
    std::string name;
    std::uint32_t pixel_offset{}; // Offset relative to the image-resource region.
    std::uint32_t width{};
    std::uint32_t height{};
    bool extent_is_valid{};
    // Retained verbatim until their runtime meanings are traced.
    std::array<std::uint8_t, 20> opaque_prefix{};
    std::array<std::uint8_t, 64> raw_name_field{};
    std::array<std::uint8_t, 12> opaque_suffix{};
};

struct ResourceCatalog {
    std::uint32_t region_file_offset{};
    std::uint32_t region_size{};
    std::uint32_t rock26_offset{};
    std::uint32_t rock26_count{};
    std::uint32_t metadata_offset{};
    int metadata_alignment{};
    std::size_t valid_image_count{};
    std::vector<ImageResource> images;

    static ResourceCatalog parse(std::span<const std::uint8_t> firmware,
                                 std::uint32_t region_file_offset,
                                 std::uint32_t region_size);

    // Writes extracted BMPs plus assets.tsv, which maps names and metadata
    // entries to ROCK26 indices and absolute firmware pixel offsets.
    std::size_t export_bitmaps(std::span<const std::uint8_t> firmware,
                               const std::filesystem::path& output_directory) const;

    // Applies same-size exported RGB565 BMP replacements to a new firmware
    // copy. Resource offsets and metadata remain fixed; unsupported or resized
    // replacements fail before the output image is written.
    std::size_t apply_bitmaps(std::span<const std::uint8_t> firmware,
                              const std::filesystem::path& input_directory,
                              const std::filesystem::path& output_firmware) const;
};

} // namespace echo::resources
