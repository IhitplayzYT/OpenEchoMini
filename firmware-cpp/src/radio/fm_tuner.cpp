#include "echo/radio/fm_tuner.hpp"

#include <algorithm>

namespace echo::radio {

FmBand fm_band_for_region(const std::uint8_t region_code) noexcept {
    return region_code == 2 ? FmBand{7600, 9000} : FmBand{8750, 10800};
}

FmTunerState::FmTunerState(const std::uint8_t region_code) noexcept
    : band_(fm_band_for_region(region_code)), frequency_(band_.minimum_tenths_mhz) {}

void FmTunerState::set_frequency(const std::uint16_t frequency) noexcept {
    frequency_ = std::clamp(frequency, band_.minimum_tenths_mhz,
                            band_.maximum_tenths_mhz);
}

void FmTunerState::step(const bool upward, const std::uint16_t amount) noexcept {
    const auto candidate = upward
        ? static_cast<std::uint32_t>(frequency_) + amount
        : (frequency_ >= amount ? static_cast<std::uint32_t>(frequency_ - amount) : 0U);
    if (candidate > band_.maximum_tenths_mhz) {
        frequency_ = band_.minimum_tenths_mhz;
    } else if (candidate < band_.minimum_tenths_mhz) {
        frequency_ = band_.maximum_tenths_mhz;
    } else {
        frequency_ = static_cast<std::uint16_t>(candidate);
    }
}

void FmTunerState::snap_to_tenth_mhz() noexcept {
    set_frequency(static_cast<std::uint16_t>((frequency_ / 10U) * 10U));
}

void FmTunerState::set_from_slider_delta(const std::int32_t slider_delta) noexcept {
    constexpr std::int64_t slider_reciprocal = 0x51eb851f;
    constexpr std::uint64_t divide_by_ten_reciprocal = 0xcccccccdULL;
    const auto span = static_cast<std::int32_t>(band_.maximum_tenths_mhz -
                                                band_.minimum_tenths_mhz);
    const auto product = static_cast<std::int64_t>(slider_delta) * span;
    const auto scaled_high = static_cast<std::int32_t>((product * slider_reciprocal) >> 32);
    const auto scaled_delta = (scaled_high >> 6) - (scaled_high >> 31);
    const auto raw = static_cast<std::uint32_t>(static_cast<std::int32_t>(band_.minimum_tenths_mhz) +
                                                scaled_delta);
    const auto rounded = static_cast<std::uint32_t>(
        (divide_by_ten_reciprocal * raw) >> 35) * 10U;
    if (rounded > band_.maximum_tenths_mhz) {
        frequency_ = band_.minimum_tenths_mhz;
    } else if (rounded < band_.minimum_tenths_mhz) {
        frequency_ = band_.maximum_tenths_mhz;
    } else {
        frequency_ = static_cast<std::uint16_t>(rounded);
    }
}

bool FmTunerState::remember_station(const std::uint16_t frequency) noexcept {
    if (frequency < band_.minimum_tenths_mhz || frequency > band_.maximum_tenths_mhz) {
        return false;
    }
    const auto bucket = frequency / 10U;
    for (std::size_t i = 0; i < preset_count_; ++i) {
        if (presets_[i] / 10U == bucket) {
            return false;
        }
    }
    if (preset_count_ == presets_.size()) {
        return false;
    }
    presets_[preset_count_++] = frequency;
    preset_cursor_ = preset_count_ - 1;
    return true;
}

bool FmTunerState::select_preset(const bool upward) noexcept {
    if (preset_count_ == 0) {
        return false;
    }
    for (std::size_t attempt = 0; attempt < preset_count_; ++attempt) {
        if (upward) {
            preset_cursor_ = (preset_cursor_ + 1) % preset_count_;
        } else {
            preset_cursor_ = preset_cursor_ == 0 ? preset_count_ - 1 : preset_cursor_ - 1;
        }
        const auto candidate = presets_[preset_cursor_];
        if (candidate >= band_.minimum_tenths_mhz &&
            candidate <= band_.maximum_tenths_mhz) {
            frequency_ = candidate;
            return true;
        }
    }
    frequency_ = band_.minimum_tenths_mhz;
    return false;
}

std::uint16_t FmTunerState::preset(const std::size_t index) const noexcept {
    return index < preset_count_ ? presets_[index] : 0;
}

} // namespace echo::radio
