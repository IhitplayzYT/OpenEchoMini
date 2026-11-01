#pragma once

#include "echo/storage/block_reader.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace echo::storage {

class TwoSlotBlockDeviceRegistry final : public SectorBackend {
public:
    using ReadCallback = std::function<int(
        std::uint8_t, std::uint32_t, std::uint32_t, std::span<std::uint8_t>)>;
    using WriteCallback = std::function<int(
        std::uint8_t, std::uint32_t, std::uint32_t,
        std::span<const std::uint8_t>)>;

    struct Callbacks {
        std::uint16_t key{};
        ReadCallback read;
        WriteCallback write;
    };

    // Descriptor slots are indexed 0 and 1, matching the observed 0x28 stride.
    void set_slot(std::size_t slot, Callbacks callbacks);

    [[nodiscard]] bool read_sectors(
        std::uint32_t device_key, std::uint32_t first_sector,
        std::uint32_t sector_count, std::span<std::uint8_t> output) override;
    [[nodiscard]] bool write_sectors(
        std::uint32_t device_key, std::uint32_t first_sector,
        std::uint32_t sector_count, std::span<const std::uint8_t> input);

private:
    [[nodiscard]] const Callbacks* find(std::uint32_t device_key) const noexcept;

    std::array<Callbacks, 2> slots_{};
};

} // namespace echo::storage
