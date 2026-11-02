#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

inline constexpr std::size_t kModule0BrowserCatalogRecordBytes = 0x800;
inline constexpr std::size_t kModule0BrowserCatalogMetadataBytes = 0x14;
inline constexpr std::size_t kModule0BrowserCatalogBlockBytes = 0x100;

struct Module0BrowserCatalogRecord {
    std::array<std::uint8_t, kModule0BrowserCatalogMetadataBytes> leading_bytes{};
    std::array<std::array<std::uint8_t, kModule0BrowserCatalogBlockBytes>, 4>
        payload_blocks{};
};

// Decodes the fixed record shape read by the category-0/1/3/4/5/100 path in
// FUN_0305dfaa. Bytes between the leading fields and payload blocks are opaque.
[[nodiscard]] std::optional<Module0BrowserCatalogRecord>
decode_module0_browser_catalog_record(
    std::span<const std::uint8_t> bytes) noexcept;

} // namespace echo::storage
