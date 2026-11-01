#include "echo/calendar/month_navigation.hpp"

#include <algorithm>

namespace echo::calendar {

MonthNavigation::MonthNavigation(const YearMonth initial) noexcept {
    set(initial);
}

void MonthNavigation::set(YearMonth value) noexcept {
    selected_.year = std::clamp(value.year, minimum_year, maximum_year);
    selected_.month = std::clamp<std::uint8_t>(value.month, 1, 12);
}

void MonthNavigation::step_month(const int direction) noexcept {
    if (direction > 0) {
        if (selected_.month == 12) {
            if (selected_.year < maximum_year) {
                ++selected_.year;
                selected_.month = 1;
            }
        } else {
            ++selected_.month;
        }
    } else if (direction < 0) {
        if (selected_.month == 1) {
            if (selected_.year > minimum_year) {
                --selected_.year;
                selected_.month = 12;
            }
        } else {
            --selected_.month;
        }
    }
}

void MonthNavigation::step_year(const int direction) noexcept {
    if (direction > 0 && selected_.year < maximum_year) {
        ++selected_.year;
    } else if (direction < 0 && selected_.year > minimum_year) {
        --selected_.year;
    }
}

} // namespace echo::calendar
