#pragma once

#include <cstdint>

namespace echo::audio {

struct Module80ApeAdaptiveModelState {
    std::uint32_t k{};
    std::uint32_t average{};
};

// Applies the common post-symbol average and k-boundary update from
// FUN_01002ee4. Returns false when k has no observed adjacent boundary pair.
[[nodiscard]] bool update_module80_ape_adaptive_model(
    Module80ApeAdaptiveModelState& state,
    std::uint32_t decoded_unsigned_value) noexcept;

} // namespace echo::audio
