#include "echo/audio/module73_layer3_block_scale.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace echo::audio {

namespace {

constexpr std::array<std::array<std::int32_t, 36>, 2> kBlockTables{{
#include "module73_layer3_block_tables.inc"
}};

void multiply_wrapped(std::int32_t& value, const std::int32_t factor) noexcept {
    const auto product = static_cast<std::uint32_t>(value) *
                         static_cast<std::uint32_t>(factor);
    value = std::bit_cast<std::int32_t>(product);
}

void shift_q12_wrapped(std::int32_t& value) noexcept {
    value = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) << 12U);
}

} // namespace

void apply_module73_layer3_block_scale(
    const std::span<std::int32_t, 36> values,
    const std::uint8_t block_type) noexcept {
    if (block_type == 0) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            multiply_wrapped(values[i], kBlockTables[0][i]);
        }
        return;
    }

    if (block_type == 1) {
        for (std::size_t i = 0; i < 18; ++i) {
            multiply_wrapped(values[i], kBlockTables[0][i]);
        }
        for (std::size_t i = 18; i < 24; ++i) shift_q12_wrapped(values[i]);
        for (std::size_t i = 24; i < 30; ++i) {
            multiply_wrapped(values[i], kBlockTables[1][i - 18]);
        }
        for (std::size_t i = 30; i < values.size(); ++i) values[i] = 0;
        return;
    }

    if (block_type == 3) {
        for (std::size_t i = 0; i < 6; ++i) values[i] = 0;
        for (std::size_t i = 6; i < 12; ++i) {
            multiply_wrapped(values[i], kBlockTables[1][i - 6]);
        }
        for (std::size_t i = 12; i < 18; ++i) shift_q12_wrapped(values[i]);
        for (std::size_t i = 18; i < values.size(); ++i) {
            multiply_wrapped(values[i], kBlockTables[0][i]);
        }
    }
}

} // namespace echo::audio
