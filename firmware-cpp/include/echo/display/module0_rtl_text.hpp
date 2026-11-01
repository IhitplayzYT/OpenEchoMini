#pragma once

#include <cstddef>
#include <optional>
#include <span>

namespace echo::display {

[[nodiscard]] bool module0_marks_rtl_code_unit(char16_t value) noexcept;

// Reproduces module 0's bounded Hebrew-run reordering heuristic. `input`
// must contain a UTF-16 NUL terminator within the first 200 code units; the
// output span must hold the copied text and terminator. The returned count
// excludes the terminator. This is not the Unicode Bidirectional Algorithm.
[[nodiscard]] std::optional<std::size_t> reorder_module0_hebrew_runs(
    std::span<const char16_t> input,
    std::span<char16_t> output) noexcept;

} // namespace echo::display
