#pragma once

#include <cstdint>

namespace echo::calendar {

struct YearMonth {
    std::uint16_t year = 2000;
    std::uint8_t month = 1; // 1 through 12
};

// Module 6's month selector wraps across years and clamps to 2000–2098.
// Day counts and the firmware's packed calendar tables are outside this model.
class MonthNavigation {
public:
    static constexpr std::uint16_t minimum_year = 2000;
    static constexpr std::uint16_t maximum_year = 2098;

    explicit MonthNavigation(YearMonth initial = {}) noexcept;

    [[nodiscard]] YearMonth selected() const noexcept { return selected_; }
    void set(YearMonth value) noexcept;
    void step_month(int direction) noexcept;
    void step_year(int direction) noexcept;

private:
    YearMonth selected_{};
};

} // namespace echo::calendar
