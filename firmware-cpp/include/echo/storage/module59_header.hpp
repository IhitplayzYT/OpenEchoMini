#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

enum class Module59HeaderStatus : std::uint8_t {
    valid_prefix,
    short_input,
    invalid_magic,
    invalid_directory_marker,
};

struct Module59HeaderPrefix {
    std::uint32_t magic{};
    std::uint16_t type_primary{};
    std::uint16_t type_secondary{};
    std::uint8_t format_tag{};
    std::uint32_t header_word_at_22{};
    std::uint16_t field6{};
    std::uint16_t field7{};
    std::array<std::uint16_t, 4> fields8_to_11{};
    std::uint32_t field12{};
    std::uint32_t field14{};
    std::size_t bytes_consumed{};
    bool secondary_metadata_probe_required{};
};

struct Module59HeaderParseResult {
    Module59HeaderStatus status{Module59HeaderStatus::short_input};
    Module59HeaderPrefix header{};
};

// Parses the fixed prefix accepted by FUN_0308d898. The function does not
// perform its later secondary-metadata probe or any stream transforms.
[[nodiscard]] Module59HeaderParseResult parse_module59_header_prefix(
    std::span<const std::uint8_t> bytes);

} // namespace echo::storage
