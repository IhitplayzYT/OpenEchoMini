#include "echo/audio/vorbis_floor1_synthesis.hpp"
#include "echo/audio/vorbis_floor1_inverse_db.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace echo::audio {
namespace {

bool apply_gain(std::int32_t& sample, std::uint16_t y, std::uint8_t multiplier,
                std::span<const std::int32_t> inverse_db_table) noexcept {
    const auto index = static_cast<std::size_t>(y) * multiplier;
    if (index >= inverse_db_table.size()) return false;
    const auto scaled = static_cast<std::int64_t>(sample >> 6) * inverse_db_table[index];
    if (scaled < std::numeric_limits<std::int32_t>::min() ||
        scaled > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    sample = static_cast<std::int32_t>(scaled);
    return true;
}

} // namespace

std::optional<std::vector<std::int32_t>> render_vorbis_floor1_spectrum(
    std::span<const std::int32_t> residue_spectrum,
    const VorbisFloor1Setup& setup,
    const VorbisFloor1Packet& floor,
    std::uint8_t multiplier,
    std::span<const std::int32_t> inverse_db_table) {
    if (multiplier == 0 || setup.x_list.size() < 2 ||
        floor.y.size() != setup.x_list.size() || floor.active.size() != setup.x_list.size() ||
        setup.sorted_post_order.size() != setup.x_list.size() ||
        setup.x_list[0] != 0 || setup.x_list[1] > residue_spectrum.size()) {
        return std::nullopt;
    }

    std::vector<bool> seen(setup.x_list.size(), false);
    for (const auto post : setup.sorted_post_order) {
        if (post >= setup.x_list.size() || seen[post]) return std::nullopt;
        seen[post] = true;
    }
    if (setup.sorted_post_order[0] != 0) return std::nullopt;

    std::vector<std::int32_t> output(residue_spectrum.begin(), residue_spectrum.end());
    if (!floor.present) {
        std::fill(output.begin(), output.end(), 0);
        return output;
    }
    if (!floor.active[0] || !floor.active[1]) return std::nullopt;

    std::size_t x0 = 0;
    auto y0 = static_cast<std::int32_t>(floor.y[0]);
    for (std::size_t sorted = 1; sorted < setup.sorted_post_order.size(); ++sorted) {
        const auto post = setup.sorted_post_order[sorted];
        if (!floor.active[post]) continue;
        const auto x1 = static_cast<std::size_t>(setup.x_list[post]);
        const auto y1 = static_cast<std::int32_t>(floor.y[post]);
        if (x1 < x0 || x1 > output.size()) return std::nullopt;

        const auto adx = static_cast<std::int32_t>(x1 - x0);
        const auto dy = y1 - y0;
        if (adx == 0) return std::nullopt;
        const auto base = dy / adx;
        const auto step = dy < 0 ? -1 : 1;
        auto remainder = std::abs(dy) - std::abs(base) * adx;
        auto error = 0;
        auto y = y0;

        for (auto x = x0; x < x1; ++x) {
            if (y < 0 || y > std::numeric_limits<std::uint16_t>::max() ||
                !apply_gain(output[x], static_cast<std::uint16_t>(y), multiplier,
                            inverse_db_table)) {
                return std::nullopt;
            }
            error += remainder;
            if (error >= adx) {
                error -= adx;
                y += step;
            }
            y += base;
        }
        x0 = x1;
        y0 = y1;
    }

    if (x0 > output.size() || y0 < 0 || y0 > std::numeric_limits<std::uint16_t>::max()) {
        return std::nullopt;
    }
    for (auto x = x0; x < output.size(); ++x) {
        if (!apply_gain(output[x], static_cast<std::uint16_t>(y0), multiplier,
                        inverse_db_table)) {
            return std::nullopt;
        }
    }
    return output;
}

std::optional<std::vector<std::int32_t>> render_vorbis_floor1_spectrum(
    std::span<const std::int32_t> residue_spectrum,
    const VorbisFloor1Setup& setup,
    const VorbisFloor1Packet& floor,
    std::uint8_t multiplier) {
    return render_vorbis_floor1_spectrum(
        residue_spectrum, setup, floor, multiplier, kVorbisFloor1InverseDbQ22);
}

} // namespace echo::audio
