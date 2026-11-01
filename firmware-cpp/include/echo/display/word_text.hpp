#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::display {

// Models module 27's currently unreferenced FUN_0307be00 helper. When
// preserve_order is true, 16-bit units are copied unchanged; otherwise each
// unit's bytes are swapped. The bounded span API prevents the source routine's
// caller-supplied count from overrunning either buffer.
[[nodiscard]] std::size_t copy_or_swap_words(
    std::span<std::uint16_t> destination,
    std::span<const std::uint16_t> source, std::size_t count,
    bool preserve_order) noexcept;

} // namespace echo::display
