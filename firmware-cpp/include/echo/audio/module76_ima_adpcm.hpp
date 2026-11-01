#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

struct Module76ImaAdpcmState {
    std::int32_t predictor{};
    std::int32_t step_index{};
};

// Decodes one low-four-bit IMA ADPCM code and updates its predictor/index.
// Uses the exact 89-entry step and 8-entry index tables extracted from
// module_76.data.bin at 0x01024c2c and 0x01024c0c.
[[nodiscard]] std::int16_t decode_module76_ima_adpcm_nibble(
    std::uint8_t nibble, Module76ImaAdpcmState& state) noexcept;

struct Module76ImaAdpcmBlock {
    // Channel-separated samples; the firmware's final PCM buffer layout is
    // handled by a caller outside this bounded block decoder.
    std::vector<std::vector<std::int16_t>> channels;
};

// Decodes WAVE IMA ADPCM block framing: one 4-byte predictor/index header per
// channel, followed by 4-byte coded groups per channel and low-nibble-first
// sample reconstruction. Only mono and stereo are accepted by module 76.
[[nodiscard]] std::optional<Module76ImaAdpcmBlock> decode_module76_ima_adpcm_block(
    std::span<const std::uint8_t> block, std::uint8_t channel_count);

} // namespace echo::audio
