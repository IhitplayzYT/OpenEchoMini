#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::radio {

struct FmBand {
    std::uint16_t minimum_tenths_mhz;
    std::uint16_t maximum_tenths_mhz;
};

// Module 60 uses 76–90 MHz for region code 2 and 87.5–108 MHz otherwise.
FmBand fm_band_for_region(std::uint8_t region_code) noexcept;

class FmTunerState {
public:
    static constexpr std::size_t preset_capacity = 30;

    explicit FmTunerState(std::uint8_t region_code = 0) noexcept;

    FmBand band() const noexcept { return band_; }
    std::uint16_t frequency() const noexcept { return frequency_; }
    void set_frequency(std::uint16_t frequency) noexcept;
    void step(bool upward, std::uint16_t amount = 10) noexcept;
    void snap_to_tenth_mhz() noexcept;
    // slider_delta is the source's slider coordinate minus its 20-unit origin.
    void set_from_slider_delta(std::int32_t slider_delta) noexcept;

    bool remember_station(std::uint16_t frequency) noexcept;
    bool select_preset(bool upward) noexcept;
    std::size_t preset_count() const noexcept { return preset_count_; }
    std::uint16_t preset(std::size_t index) const noexcept;

private:
    FmBand band_;
    std::uint16_t frequency_;
    std::array<std::uint16_t, preset_capacity> presets_{};
    std::size_t preset_count_ = 0;
    std::size_t preset_cursor_ = 0;
};

} // namespace echo::radio
