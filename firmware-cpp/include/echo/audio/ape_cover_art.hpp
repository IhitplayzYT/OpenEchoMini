#pragma once

#include "echo/resources/embedded_image.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace echo::audio {

struct ApeCoverArtItem {
    std::uint32_t flags{};
    std::string_view filename;
    std::span<const std::uint8_t> image_data;
    std::optional<resources::EmbeddedImageSignature> signature;
};

struct ApeTagView {
    std::uint32_t version{};
    std::uint32_t tag_size{};
    std::uint32_t item_count{};
    std::uint32_t flags{};
    std::span<const std::uint8_t> items;
};

// Models module 18's footer-at-EOF check and its bounded item region. The
// firmware reads but does not validate the version field in this path.
[[nodiscard]] std::optional<ApeTagView> module18_locate_ape_tag(
    std::span<const std::uint8_t> file) noexcept;

// Scans an already bounded APEv2 item region for the exact front-cover key.
// The footer and tag start offset are resolved by the caller.
[[nodiscard]] std::optional<ApeCoverArtItem> find_ape_front_cover_item(
    std::span<const std::uint8_t> items,
    std::uint32_t item_count) noexcept;

} // namespace echo::audio
