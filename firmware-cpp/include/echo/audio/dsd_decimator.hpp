#pragma once

#include "echo/audio/dsd_decimator_lookup.hpp"

#include <array>
#include <bit>
#include <cstdint>

namespace echo::audio {

enum class DsdHistoryDirection {
    // Module 81 (FUN_01003798) visits the newest byte, then older bytes.
    newest_to_older,
    // Module 82 (FUN_010035ec) visits the newest byte, then newer ring slots.
    newest_to_newer,
};

struct DsdDecimatorHistory {
    std::array<std::uint8_t, 64> bytes{};
    std::uint8_t cursor{63};

    void push(std::uint8_t value) noexcept {
        cursor = static_cast<std::uint8_t>((cursor + 1U) & 0x3fU);
        bytes[cursor] = value;
    }
};

// Returns the firmware's table-driven group sum before its final conditional
// replacement/output-store step. Unsigned accumulation preserves ARM's
// modulo-2^32 addition without signed-overflow behavior in C++.
[[nodiscard]] inline std::int32_t accumulate_dsd_history(
    const DsdDecimatorHistory& history, const DsdDecimatorLookup& lookup,
    DsdHistoryDirection direction) {
    std::uint32_t sum = 0;
    for (std::size_t group = 0; group < lookup.groups; ++group) {
        const auto delta = static_cast<std::uint8_t>(group & 0x3fU);
        const auto ring_index = direction == DsdHistoryDirection::newest_to_older
            ? static_cast<std::uint8_t>((static_cast<unsigned>(history.cursor) + 64U - delta) & 0x3fU)
            : static_cast<std::uint8_t>((history.cursor + delta) & 0x3fU);
        const auto value = std::bit_cast<std::uint32_t>(
            lookup.at(history.bytes[ring_index], group));
        sum += value;
    }
    return std::bit_cast<std::int32_t>(sum);
}

} // namespace echo::audio
