#pragma once

#include <array>
#include <cstdint>

namespace echo::audio {

// Observed values copied by FUN_0100378c into context word offsets 0x11 and
// 0x15. Keep the field names neutral until FUN_010030b8's full predictor
// arithmetic and history semantics are ported.
struct Module80ApePredictorStateSeed {
    std::array<std::array<std::int32_t, 4>, 2> channel_seed_words{};
    std::array<std::array<std::int32_t, 5>, 2> adaptive_words{};
};

[[nodiscard]] constexpr Module80ApePredictorStateSeed
module80_ape_predictor_state_seed() noexcept {
    constexpr std::array<std::int32_t, 4> seed{360, 317, -109, 98};
    return {{{seed, seed}}, {}};
}

} // namespace echo::audio
