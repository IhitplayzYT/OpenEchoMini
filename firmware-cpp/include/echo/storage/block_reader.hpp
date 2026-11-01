#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::storage {

class SectorBackend {
public:
    virtual ~SectorBackend() = default;
    [[nodiscard]] virtual bool read_sectors(
        std::uint32_t device_key, std::uint32_t first_sector,
        std::uint32_t sector_count, std::span<std::uint8_t> output) = 0;
};

struct BlockReaderConfig {
    std::uint32_t cached_device_key{0x30001};
    std::uint32_t retry_device_key{0x30000};
    std::uint32_t offset_device_key{0x30000};
    std::uint32_t base_sector_offset{};
    bool apply_base_sector_offset{};
};

// Models the firmware's 0x200-byte block read helper. Partial reads on one
// configured device use three sector slots and select the nearest cached LBA.
class BlockByteReader {
public:
    static constexpr std::size_t sector_size = 0x200;
    static constexpr std::size_t cache_slots = 3;

    explicit BlockByteReader(SectorBackend& backend,
                             BlockReaderConfig config = {}) noexcept;

    [[nodiscard]] bool read(std::uint32_t device_key,
                            std::uint64_t byte_offset,
                            std::span<std::uint8_t> output);

private:
    struct CacheSlot {
        std::uint32_t sector{0xffffffff};
        std::array<std::uint8_t, sector_size> bytes{};
    };

    [[nodiscard]] bool read_one_sector(std::uint32_t device_key,
                                       std::uint32_t sector,
                                       std::span<std::uint8_t> output);
    [[nodiscard]] std::size_t choose_slot(std::uint32_t sector) const noexcept;

    SectorBackend& backend_;
    BlockReaderConfig config_;
    std::array<CacheSlot, cache_slots> cache_{};
};

} // namespace echo::storage
