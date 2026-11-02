#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::audio {

struct VorbisInverseTransformPlan {
    std::uint32_t block_size{};
    std::uint32_t half_block{};
    std::uint8_t transform_shift{};
    std::uint16_t table_stride{};
};

// Describes the block-size parameters selected by FUN_0100accc. The current
// transform implementation accepts power-of-two blocks from 64 through 8192.
[[nodiscard]] std::optional<VorbisInverseTransformPlan>
plan_vorbis_inverse_transform(std::uint32_t block_size) noexcept;

[[nodiscard]] std::optional<std::uint32_t> vorbis_transform_bit_reversal_index(
    const VorbisInverseTransformPlan& plan,
    std::uint32_t index) noexcept;

[[nodiscard]] bool reorder_vorbis_transform_pairs(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept;

// Ports FUN_0100a07c's first fixed-point inverse-transform rotation stage.
[[nodiscard]] bool apply_vorbis_inverse_transform_rotation(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept;

// Ports the repeated 32-value cleanup block, including its two 16-value
// kernels, called at the end of FUN_0100a68c.
[[nodiscard]] bool apply_vorbis_inverse_transform_cleanup(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept;

// Ports FUN_0100a68c's variable-stage FFT butterflies and final cleanup pass.
[[nodiscard]] bool apply_vorbis_inverse_transform_fft(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept;

[[nodiscard]] bool apply_vorbis_inverse_transform_recombination(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept;

// Ports the general stride branch (64–2048) and special 4096/8192 cases of
// FUN_0100a980.
[[nodiscard]] bool apply_vorbis_inverse_transform_final_rotation(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept;

// Runs all currently reconstructed inverse-transform stages for block sizes
// 64–8192, including the dedicated large-block final-rotation branches.
[[nodiscard]] bool apply_vorbis_inverse_transform_stages(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept;

} // namespace echo::audio
