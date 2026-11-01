#pragma once

#include <cstdint>
#include <optional>

namespace echo::audio {

// Reproduces the bounded packed-table magnitude scaling in FUN_010055cc.
// The selected table is the module 73 Layer III fixed-point lookup observed
// in module_73.data.bin; out-of-table magnitude values return nullopt.
[[nodiscard]] std::optional<std::uint32_t> module73_layer3_requantize(
    std::uint32_t quantized_magnitude,
    std::int32_t exponent) noexcept;

} // namespace echo::audio
