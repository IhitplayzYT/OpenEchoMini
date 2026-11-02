#include "echo/storage/module0_browser_catalog_record.hpp"

#include <algorithm>
#include <cstddef>

namespace echo::storage {

std::optional<Module0BrowserCatalogRecord>
decode_module0_browser_catalog_record(
    const std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < kModule0BrowserCatalogRecordBytes) return std::nullopt;

    Module0BrowserCatalogRecord record;
    std::copy_n(bytes.begin(), record.leading_bytes.size(),
                record.leading_bytes.begin());

    for (std::size_t block = 0; block < record.payload_blocks.size(); ++block) {
        const auto offset = 0x400 + block * kModule0BrowserCatalogBlockBytes;
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                    kModule0BrowserCatalogBlockBytes,
                    record.payload_blocks[block].begin());
    }
    return record;
}

} // namespace echo::storage
