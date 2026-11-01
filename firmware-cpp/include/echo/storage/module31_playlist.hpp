#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace echo::storage {

inline constexpr std::size_t module31_max_playlist_path_units = 0x103;

enum class Module31BomKind : std::uint8_t {
    none,
    utf8,
    utf16_little_endian,
    utf16_big_endian,
};

struct Module31Bom {
    Module31BomKind kind{};
    std::uint8_t bytes_to_skip{};
};

[[nodiscard]] Module31Bom detect_module31_bom(
    std::span<const std::uint8_t> prefix) noexcept;

enum class Module31PlaylistRoot : std::uint8_t {
    playlist_context,
    opaque_path_helper,
};

struct Module31PlaylistAttempt {
    std::u16string path;
    Module31PlaylistRoot root{};
    bool explicit_path_mode{};
};

struct Module31PlaylistLinePlan {
    bool ignored{};
    std::u16string trimmed_line;
    std::array<Module31PlaylistAttempt, 3> attempts{};
    std::uint8_t attempt_count{};
};

// Models M3U line whitespace/comment handling and the observed ordered path
// resolution attempts. It does not resolve paths or open referenced files.
[[nodiscard]] Module31PlaylistLinePlan plan_module31_playlist_line(
    std::u16string_view decoded_line);

} // namespace echo::storage
