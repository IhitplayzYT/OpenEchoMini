#include "echo/storage/block_device_registry.hpp"

#include <stdexcept>
#include <utility>

namespace echo::storage {

void TwoSlotBlockDeviceRegistry::set_slot(std::size_t slot, Callbacks callbacks) {
    if (slot >= slots_.size()) throw std::out_of_range("block-device slot must be 0 or 1");
    slots_[slot] = std::move(callbacks);
}

const TwoSlotBlockDeviceRegistry::Callbacks*
TwoSlotBlockDeviceRegistry::find(std::uint32_t device_key) const noexcept {
    const auto descriptor_key = static_cast<std::uint16_t>(device_key >> 16U);
    for (const auto& slot : slots_) {
        if (slot.key == descriptor_key) return &slot;
    }
    return nullptr;
}

bool TwoSlotBlockDeviceRegistry::read_sectors(
    std::uint32_t device_key, std::uint32_t first_sector,
    std::uint32_t sector_count, std::span<std::uint8_t> output) {
    const auto* callbacks = find(device_key);
    if (callbacks == nullptr || !callbacks->read) return false;
    return callbacks->read(static_cast<std::uint8_t>(device_key), first_sector,
                           sector_count, output) == 0;
}

bool TwoSlotBlockDeviceRegistry::write_sectors(
    std::uint32_t device_key, std::uint32_t first_sector,
    std::uint32_t sector_count, std::span<const std::uint8_t> input) {
    const auto* callbacks = find(device_key);
    if (callbacks == nullptr || !callbacks->write) return false;
    return callbacks->write(static_cast<std::uint8_t>(device_key), first_sector,
                            sector_count, input) == 0;
}

} // namespace echo::storage
