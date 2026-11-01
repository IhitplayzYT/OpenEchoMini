#include "echo/display/module0_rtl_text.hpp"

#include <algorithm>
#include <array>

namespace echo::display {
namespace {

constexpr std::size_t kLocalCapacity = 200;

void reverse_range(std::array<char16_t, kLocalCapacity>& text,
                   const std::size_t begin,
                   const std::size_t end) noexcept {
    std::reverse(text.begin() + static_cast<std::ptrdiff_t>(begin),
                 text.begin() + static_cast<std::ptrdiff_t>(end));
}

} // namespace

bool module0_marks_rtl_code_unit(const char16_t value) noexcept {
    return (value >= 0x05b0 && value <= 0x05b9) ||
           (value >= 0x05bb && value <= 0x05c3) ||
           (value >= 0x05d0 && value <= 0x05ea) ||
           (value >= 0x05f0 && value <= 0x05f4);
}

std::optional<std::size_t> reorder_module0_hebrew_runs(
    const std::span<const char16_t> input,
    const std::span<char16_t> output) noexcept {
    const auto limit = std::min(input.size(), kLocalCapacity);
    const auto terminator = std::find(input.begin(), input.begin() +
        static_cast<std::ptrdiff_t>(limit), u'\0');
    if (terminator == input.begin() + static_cast<std::ptrdiff_t>(limit)) {
        return std::nullopt;
    }

    const auto count = static_cast<std::size_t>(terminator - input.begin());
    if (output.size() < count + 1) return std::nullopt;

    std::array<char16_t, kLocalCapacity> text{};
    std::copy_n(input.begin(), count + 1, text.begin());

    auto first_rtl = std::size_t{0};
    while (first_rtl < count && !module0_marks_rtl_code_unit(text[first_rtl])) {
        ++first_rtl;
    }
    if (first_rtl == count) {
        std::copy_n(text.begin(), count + 1, output.begin());
        return count;
    }

    auto last_rtl = std::size_t{0};
    for (std::size_t i = 0; i < count; ++i) {
        if (module0_marks_rtl_code_unit(text[i])) last_rtl = i;
    }

    reverse_range(text, first_rtl, last_rtl + 1);

    auto run_start = std::size_t{0};
    auto run_length = std::size_t{0};
    for (std::size_t i = first_rtl; i <= last_rtl; ++i) {
        const auto rtl = module0_marks_rtl_code_unit(text[i]);
        if (rtl || text[i] == u' ') {
            if (run_start != 0 && run_length > 1) {
                reverse_range(text, run_start, run_start + run_length);
            }
            run_start = 0;
            run_length = 0;
        } else {
            if (run_start == 0) run_start = i;
            ++run_length;
        }
    }

    std::copy_n(text.begin(), count + 1, output.begin());
    return count;
}

} // namespace echo::display
