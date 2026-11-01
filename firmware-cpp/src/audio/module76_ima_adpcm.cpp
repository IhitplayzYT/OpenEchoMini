#include "echo/audio/module76_ima_adpcm.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace echo::audio {
namespace {

constexpr std::array<std::int32_t, 89> kStepTable{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
    143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
    4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767,
};

constexpr std::array<std::int32_t, 8> kIndexAdjustment{
    -1, -1, -1, -1, 2, 4, 6, 8,
};

} // namespace

std::int16_t decode_module76_ima_adpcm_nibble(
    std::uint8_t nibble, Module76ImaAdpcmState& state) noexcept {
    if (state.step_index < 0 || state.step_index >= static_cast<std::int32_t>(kStepTable.size())) {
        return 0;
    }

    const auto code = static_cast<std::uint8_t>(nibble & 0x0fU);
    const auto step = kStepTable[static_cast<std::size_t>(state.step_index)];
    std::int32_t difference = step >> 3;
    if ((code & 1U) != 0) difference += step >> 2;
    if ((code & 2U) != 0) difference += step >> 1;
    if ((code & 4U) != 0) difference += step;
    if ((code & 8U) != 0) difference = -difference;

    state.predictor = std::clamp(state.predictor + difference, -32768, 32767);
    state.step_index = std::clamp(
        state.step_index + kIndexAdjustment[code & 7U], 0,
        static_cast<std::int32_t>(kStepTable.size() - 1));
    return static_cast<std::int16_t>(state.predictor);
}

std::optional<Module76ImaAdpcmBlock> decode_module76_ima_adpcm_block(
    std::span<const std::uint8_t> block, std::uint8_t channel_count) {
    if (channel_count == 0 || channel_count > 2) return std::nullopt;
    const auto header_bytes = static_cast<std::size_t>(channel_count) * 4;
    if (block.size() < header_bytes) return std::nullopt;

    const auto coded_bytes = block.size() - header_bytes;
    const auto bytes_per_group = static_cast<std::size_t>(channel_count) * 4;
    if (coded_bytes % bytes_per_group != 0) return std::nullopt;
    const auto groups = coded_bytes / bytes_per_group;
    if (groups > (std::numeric_limits<std::size_t>::max() - 1) / 8) {
        return std::nullopt;
    }

    Module76ImaAdpcmBlock decoded;
    decoded.channels.resize(channel_count);
    std::vector<Module76ImaAdpcmState> states(channel_count);
    for (std::size_t channel = 0; channel < channel_count; ++channel) {
        const auto header = channel * 4;
        const auto predictor_bits = static_cast<std::uint16_t>(block[header]) |
            (static_cast<std::uint16_t>(block[header + 1]) << 8U);
        const auto step_index = static_cast<std::uint16_t>(block[header + 2]) |
            (static_cast<std::uint16_t>(block[header + 3]) << 8U);
        states[channel].predictor = static_cast<std::int16_t>(predictor_bits);
        // The firmware loads the index and reserved byte together as a
        // halfword; an invalid value then follows the scalar helper's zero path.
        states[channel].step_index = step_index;
        decoded.channels[channel].reserve(1 + groups * 8);
        decoded.channels[channel].push_back(
            static_cast<std::int16_t>(states[channel].predictor));
    }

    std::size_t cursor = header_bytes;
    while (cursor < block.size()) {
        for (std::size_t channel = 0; channel < channel_count; ++channel) {
            for (std::size_t byte_index = 0; byte_index < 4; ++byte_index) {
                const auto packed = block[cursor++];
                decoded.channels[channel].push_back(
                    decode_module76_ima_adpcm_nibble(
                        static_cast<std::uint8_t>(packed & 0x0fU), states[channel]));
                decoded.channels[channel].push_back(
                    decode_module76_ima_adpcm_nibble(
                        static_cast<std::uint8_t>(packed >> 4U), states[channel]));
            }
        }
    }
    return decoded;
}

} // namespace echo::audio
