#include "echo/audio/module73_layer3_transform.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace echo::audio {

namespace {

constexpr std::array<std::int32_t, 36> kTransformMatrix{
#include "module73_layer3_transform_matrix.inc"
};

constexpr std::array<std::int32_t, 12> kWindow{
    0x216, 0x61f, 0x9bd, 0xcb1, 0xec8, 0xfdc,
    0xfdc, 0xec8, 0xcb1, 0x9bd, 0x61f, 0x216};

std::int32_t wrapped_add(const std::int32_t left, const std::int32_t right) noexcept {
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}

std::int32_t wrapped_multiply(
    const std::int32_t left,
    const std::int32_t right) noexcept {
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) * static_cast<std::uint32_t>(right));
}

std::int32_t dot_six(
    const std::array<std::int32_t, 18>& input,
    const std::size_t input_offset,
    const std::size_t matrix_offset) noexcept {
    std::int32_t sum = 0;
    for (std::size_t i = 0; i < 6; ++i) {
        sum = wrapped_add(sum, wrapped_multiply(
            input[input_offset + i], kTransformMatrix[matrix_offset + i]));
    }
    return sum;
}

} // namespace

void transform_module73_layer3_block(
    const std::span<const std::int32_t, 18> spectral_values,
    const std::span<std::int32_t, 36> output_values) noexcept {
    std::array<std::int32_t, 18> input{};
    for (std::size_t i = 0; i < input.size(); ++i) {
        input[i] = spectral_values[i] >> 12U;
    }

    std::array<std::int32_t, 36> transformed{};
    for (std::size_t group = 0; group < 3; ++group) {
        for (std::size_t row = 0; row < 3; ++row) {
            const std::size_t row_offset = row * 12U;
            const std::int32_t first = dot_six(input, group * 6U, row_offset);
            const std::int32_t second = dot_six(input, group * 6U, row_offset + 6U);
            const std::size_t output = group * 12U;
            transformed[output + row] = first;
            transformed[output + 5U - row] =
                std::bit_cast<std::int32_t>(0U - static_cast<std::uint32_t>(first));
            transformed[output + 6U + row] = second;
            transformed[output + 11U - row] = second;
        }
    }

    for (std::size_t i = 0; i < 6; ++i) {
        output_values[i] = 0;
        output_values[6U + i] = wrapped_multiply(transformed[i] >> 12U, kWindow[i]);
        output_values[12U + i] = wrapped_add(
            wrapped_multiply(transformed[12U + i] >> 12U, kWindow[i]),
            wrapped_multiply(transformed[6U + i] >> 12U, kWindow[6U + i]));
        output_values[18U + i] = wrapped_add(
            wrapped_multiply(transformed[24U + i] >> 12U, kWindow[i]),
            wrapped_multiply(transformed[18U + i] >> 12U, kWindow[6U + i]));
        output_values[24U + i] =
            wrapped_multiply(transformed[30U + i] >> 12U, kWindow[6U + i]);
        output_values[30U + i] = 0;
    }
}

} // namespace echo::audio
