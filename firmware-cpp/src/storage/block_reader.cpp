#include "echo/storage/block_reader.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace echo::storage {

BlockByteReader::BlockByteReader(SectorBackend& backend,
                                 BlockReaderConfig config) noexcept
    : backend_(backend), config_(config) {}

bool BlockByteReader::read_one_sector(std::uint32_t device_key,
                                      std::uint32_t sector,
                                      std::span<std::uint8_t> output) {
    if (output.size() != sector_size) return false;
    if (backend_.read_sectors(device_key, sector, 1, output)) return true;
    return device_key == config_.retry_device_key
        && backend_.read_sectors(device_key, sector, 1, output);
}

std::size_t BlockByteReader::choose_slot(std::uint32_t sector) const noexcept {
    for (std::size_t i = 0; i < cache_.size(); ++i) {
        if (cache_[i].sector == 0xffffffff) return i;
    }
    const auto distance = [sector](std::uint32_t cached) {
        return cached > sector ? cached - sector : sector - cached;
    };
    std::size_t selected = 0;
    auto best = distance(cache_[0].sector);
    for (std::size_t i = 1; i < cache_.size(); ++i) {
        const auto candidate = distance(cache_[i].sector);
        if (candidate < best) {
            selected = i;
            best = candidate;
        }
    }
    return selected;
}

bool BlockByteReader::read(std::uint32_t device_key,
                           std::uint64_t byte_offset,
                           std::span<std::uint8_t> output) {
    if (output.empty()) return true;

    const auto first_sector64 = byte_offset / sector_size;
    if (first_sector64 > std::numeric_limits<std::uint32_t>::max()) return false;
    auto sector = static_cast<std::uint32_t>(first_sector64);
    if (device_key == config_.offset_device_key && config_.apply_base_sector_offset) {
        if (config_.base_sector_offset > std::numeric_limits<std::uint32_t>::max() - sector) {
            return false;
        }
        sector += config_.base_sector_offset;
    }
    auto in_sector = static_cast<std::size_t>(byte_offset % sector_size);
    auto remaining = output.size();
    std::size_t output_offset = 0;
    std::array<std::uint8_t, sector_size> scratch{};

    while (remaining != 0) {
        if (in_sector == 0 && remaining >= sector_size) {
            const auto full_count = remaining / sector_size;
            const auto count = static_cast<std::uint32_t>(std::min<std::size_t>(
                full_count, std::numeric_limits<std::uint32_t>::max()));
            auto direct = output.subspan(output_offset,
                static_cast<std::size_t>(count) * sector_size);
            const auto bytes = static_cast<std::size_t>(count) * sector_size;
            if (count > std::numeric_limits<std::uint32_t>::max() - sector
                && remaining > bytes) return false;
            bool ok = backend_.read_sectors(device_key, sector, count, direct);
            if (!ok && device_key == config_.retry_device_key) {
                ok = backend_.read_sectors(device_key, sector, count, direct);
            }
            if (!ok) return false;
            output_offset += bytes;
            remaining -= bytes;
            sector += count;
            continue;
        }

        std::span<const std::uint8_t> source;
        if (device_key == config_.cached_device_key) {
            auto found = std::find_if(cache_.begin(), cache_.end(), [sector](const CacheSlot& slot) {
                return slot.sector == sector;
            });
            if (found == cache_.end()) {
                const auto slot_index = choose_slot(sector);
                auto& slot = cache_[slot_index];
                slot.sector = 0xffffffff;
                if (!read_one_sector(device_key, sector, slot.bytes)) return false;
                slot.sector = sector;
                found = cache_.begin() + static_cast<std::ptrdiff_t>(slot_index);
            }
            source = found->bytes;
        } else {
            if (!read_one_sector(device_key, sector, scratch)) return false;
            source = scratch;
        }

        const auto count = std::min(remaining, sector_size - in_sector);
        std::copy_n(source.begin() + static_cast<std::ptrdiff_t>(in_sector), count,
                    output.begin() + static_cast<std::ptrdiff_t>(output_offset));
        output_offset += count;
        remaining -= count;
        if (remaining != 0 && sector == std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
        ++sector;
        in_sector = 0;
    }
    return true;
}

} // namespace echo::storage
