#include "echo/audio/vorbis_inverse_transform.hpp"
#include "echo/audio/vorbis_transform_tables.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <utility>

namespace echo::audio {
namespace {

bool valid_plan(const VorbisInverseTransformPlan& plan) noexcept {
    if (plan.block_size < 64 || plan.block_size > 8192 ||
        (plan.block_size & (plan.block_size - 1)) != 0 ||
        plan.half_block != plan.block_size / 2) {
        return false;
    }
    std::uint8_t log2_size = 0;
    for (auto value = plan.block_size; value > 1; value >>= 1U) ++log2_size;
    const auto shift = static_cast<std::uint8_t>(13 - log2_size);
    return plan.transform_shift == shift && plan.table_stride == (2U << shift);
}

std::int32_t wrap32(std::int64_t value) noexcept {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
}

std::int32_t add32(std::int32_t left, std::int32_t right) noexcept {
    return wrap32(static_cast<std::int64_t>(left) + right);
}

std::int32_t sub32(std::int32_t left, std::int32_t right) noexcept {
    return wrap32(static_cast<std::int64_t>(left) - right);
}

std::int32_t mul32(std::int32_t left, std::int32_t right) noexcept {
    return wrap32(static_cast<std::int64_t>(left) * right);
}

std::array<std::int32_t, 16> small_kernel16(
    std::array<std::int32_t, 16> x) noexcept {
    std::array<std::int32_t, 16> out{};
    const auto i17 = sub32(x[8], x[9]);
    const auto i1 = add32(x[9], x[8]);
    const auto i9 = sub32(x[10], x[11]);
    const auto i2 = add32(x[10], x[11]);
    const auto i3 = sub32(x[1], x[0]);
    const auto i4 = add32(x[0], x[1]);
    const auto i7 = sub32(x[3], x[2]);
    const auto i5 = add32(x[2], x[3]);
    const auto i15 = mul32(sub32(i17, i9) >> 8, 0xb5);
    auto i6 = add32(i3, i7) >> 8;
    auto i3s = sub32(i7, i3) >> 8;
    const auto i13 = mul32(add32(i17, i9) >> 8, 0xb5);
    const auto i8 = sub32(x[12], x[13]);
    const auto i12 = add32(x[12], x[13]);
    const auto i9s = sub32(x[14], x[15]);
    const auto i11 = add32(x[14], x[15]);
    const auto i7s = sub32(x[4], x[5]);
    const auto i10 = add32(x[5], x[4]);
    const auto i17s = sub32(x[7], x[6]);
    const auto i16 = add32(x[6], x[7]);
    const auto i18 = add32(i15, mul32(i6, 0xb5));
    const auto i15s = add32(i15, mul32(i6, -0xb5));
    const auto i14 = add32(i13, mul32(i3s, 0xb5));
    const auto i13s = add32(i13, mul32(i3s, -0xb5));
    i6 = add32(i8, i17s);
    const auto i8s = sub32(i8, i17s);
    const auto i3a = add32(i9s, i7s);
    const auto i9a = sub32(i9s, i7s);

    out[0] = add32(i8s, i13s);
    out[1] = sub32(i9a, i15s);
    out[2] = sub32(i8s, i13s);
    out[3] = add32(i9a, i15s);
    out[4] = sub32(i6, i18);
    out[5] = sub32(i3a, i14);
    out[6] = add32(i6, i18);
    out[7] = add32(i3a, i14);

    const auto i6a = add32(i1, i4);
    const auto i1a = sub32(i1, i4);
    const auto i7a = add32(i2, i5);
    const auto i2a = sub32(i2, i5);
    const auto i3b = add32(i12, i10);
    const auto i12b = sub32(i12, i10);
    const auto i9b = add32(i11, i16);
    const auto i11b = sub32(i11, i16);
    out[8] = add32(i12b, i2a);
    out[9] = sub32(i11b, i1a);
    out[10] = sub32(i12b, i2a);
    out[11] = add32(i11b, i1a);
    out[12] = sub32(i3b, i6a);
    out[13] = sub32(i9b, i7a);
    out[14] = add32(i3b, i6a);
    out[15] = add32(i9b, i7a);
    return out;
}

void cleanup32(std::span<std::int32_t, 32> values) noexcept {
    std::array<std::int32_t, 32> x{};
    std::copy(values.begin(), values.end(), x.begin());

    auto i2 = x[16];
    auto i1 = x[17];
    auto i3 = x[18];
    auto i4 = x[19];
    x[16] = add32(i1, i2);
    x[18] = add32(i3, i4);
    const auto i5 = x[0];
    x[17] = add32(x[0], x[1]);
    const auto i6 = x[2];
    x[19] = add32(x[2], x[3]);
    auto i1s = sub32(i2, i1) >> 8;
    auto i2s = sub32(i3, i4) >> 8;
    x[0] = add32(mul32(i1s, 0x62), mul32(i2s, -0xed));
    const auto i3s = sub32(x[1], i5) >> 8;
    x[2] = add32(mul32(i1s, 0xed), mul32(i2s, 0x62));
    i1s = sub32(x[3], i6) >> 8;
    x[1] = add32(mul32(i1s, 0x62), mul32(i3s, 0xed));
    x[3] = add32(mul32(i1s, 0xed), mul32(i3s, -0x62));

    auto i2d = sub32(x[20], x[21]);
    x[20] = add32(x[21], x[20]);
    auto i3d = sub32(x[22], x[23]);
    x[22] = add32(x[22], x[23]);
    const auto i4d = sub32(x[5], x[4]);
    x[21] = add32(x[5], x[4]);
    const auto i1d = sub32(x[7], x[6]);
    x[23] = add32(x[6], x[7]);
    x[4] = mul32(sub32(i2d, i3d) >> 8, 0xb5);
    x[5] = mul32(add32(i1d, i4d) >> 8, 0xb5);
    x[6] = mul32(add32(i2d, i3d) >> 8, 0xb5);
    x[7] = mul32(sub32(i1d, i4d) >> 8, 0xb5);

    auto i1e = x[24];
    auto i3e = x[25];
    x[24] = add32(i1e, i3e);
    auto i4e = x[26];
    auto i2e = x[27];
    x[26] = add32(i2e, i4e);
    const auto i5e = x[8];
    x[25] = add32(x[9], i5e);
    const auto i6e = x[10];
    x[27] = add32(i6e, x[11]);
    i1e = sub32(i1e, i3e) >> 8;
    i2e = sub32(i4e, i2e) >> 8;
    x[8] = add32(mul32(i1e, 0xed), mul32(i2e, -0x62));
    const auto i3f = sub32(x[9], i5e) >> 8;
    x[10] = add32(mul32(i1e, 0x62), mul32(i2e, 0xed));
    i1e = sub32(x[11], i6e) >> 8;
    x[9] = add32(mul32(i1e, 0xed), mul32(i3f, 0x62));
    x[11] = add32(mul32(i1e, 0x62), mul32(i3f, -0xed));

    i1e = x[28];
    i3e = x[29];
    x[28] = add32(i1e, i3e);
    i4e = x[30];
    i2e = x[31];
    x[30] = add32(i2e, i4e);
    x[29] = add32(x[12], x[13]);
    const auto i5f = x[15];
    x[15] = sub32(x[12], x[13]);
    x[12] = sub32(i1e, i3e);
    x[31] = add32(x[14], i5f);
    x[13] = sub32(i5f, x[14]);
    x[14] = sub32(i4e, i2e);

    std::array<std::int32_t, 16> first{};
    std::array<std::int32_t, 16> second{};
    std::copy_n(x.begin(), 16, first.begin());
    std::copy_n(x.begin() + 16, 16, second.begin());
    const auto first_out = small_kernel16(first);
    const auto second_out = small_kernel16(second);
    std::copy(first_out.begin(), first_out.end(), values.begin());
    std::copy(second_out.begin(), second_out.end(), values.begin() + 16);
}

void butterfly_forward(std::span<std::int32_t> values,
                       std::size_t high,
                       std::size_t low,
                       std::uint8_t c0,
                       std::uint8_t c1) noexcept {
    const auto a0 = values[high];
    const auto a1 = values[high + 1];
    const auto a2 = values[high + 2];
    const auto a3 = values[high + 3];
    const auto b0 = values[low];
    const auto b1 = values[low + 1];
    const auto b2 = values[low + 2];
    const auto b3 = values[low + 3];

    values[high] = add32(a1, a0);
    values[high + 2] = add32(a2, a3);
    values[high + 1] = add32(b1, b0);
    values[high + 3] = add32(b2, b3);
    const auto d0 = sub32(a3, a2) >> 8;
    const auto d1 = sub32(a0, a1) >> 8;
    values[low] = add32(mul32(d1, c1), mul32(d0, c0));
    values[low + 2] = sub32(mul32(d1, c0), mul32(d0, c1));
    const auto d2 = sub32(b1, b0) >> 8;
    const auto d3 = sub32(b3, b2) >> 8;
    values[low + 1] = add32(mul32(d3, c1), mul32(d2, c0));
    values[low + 3] = sub32(mul32(d3, c0), mul32(d2, c1));
}

void butterfly_reverse(std::span<std::int32_t> values,
                       std::size_t high,
                       std::size_t low,
                       std::uint8_t c0,
                       std::uint8_t c1) noexcept {
    const auto a0 = values[high];
    const auto a1 = values[high + 1];
    const auto a2 = values[high + 2];
    const auto a3 = values[high + 3];
    const auto b0 = values[low];
    const auto b1 = values[low + 1];
    const auto b2 = values[low + 2];
    const auto b3 = values[low + 3];

    values[high] = add32(a0, a1);
    values[high + 2] = add32(a2, a3);
    values[high + 1] = add32(b0, b1);
    values[high + 3] = add32(b2, b3);
    const auto d0 = sub32(a0, a1) >> 8;
    const auto d1 = sub32(a2, a3) >> 8;
    values[low] = sub32(mul32(d0, c0), mul32(d1, c1));
    values[low + 2] = add32(mul32(d0, c1), mul32(d1, c0));
    const auto d2 = sub32(b3, b2) >> 8;
    const auto d3 = sub32(b0, b1) >> 8;
    values[low + 1] = sub32(mul32(d2, c0), mul32(d3, c1));
    values[low + 3] = add32(mul32(d2, c1), mul32(d3, c0));
}

} // namespace

std::optional<VorbisInverseTransformPlan>
plan_vorbis_inverse_transform(std::uint32_t block_size) noexcept {
    if (block_size < 64 || block_size > 8192 ||
        (block_size & (block_size - 1)) != 0) {
        return std::nullopt;
    }

    std::uint8_t log2_size = 0;
    for (auto value = block_size; value > 1; value >>= 1U) ++log2_size;
    const auto shift = static_cast<std::uint8_t>(13 - log2_size);
    const auto stride = static_cast<std::uint16_t>(2U << shift);
    return VorbisInverseTransformPlan{
        .block_size = block_size,
        .half_block = block_size / 2,
        .transform_shift = shift,
        .table_stride = stride,
    };
}

std::optional<std::uint32_t> vorbis_transform_bit_reversal_index(
    const VorbisInverseTransformPlan& plan,
    std::uint32_t index) noexcept {
    if (!valid_plan(plan) || index >= plan.half_block) {
        return std::nullopt;
    }
    const auto high = index >> 8U;
    const auto middle = (index & 0xffU) >> 4U;
    const auto low = index & 0x0fU;
    if (high >= 16 || middle >= 16 || low >= 16) return std::nullopt;
    const auto reversed = static_cast<std::uint32_t>(
        kVorbisBitPermutationTable[high] |
        (static_cast<std::uint32_t>(kVorbisBitPermutationTable[middle]) << 4U) |
        (static_cast<std::uint32_t>(kVorbisBitPermutationTable[low]) << 8U));
    const auto result = reversed >> plan.transform_shift;
    if (result >= plan.half_block) return std::nullopt;
    return result;
}

bool reorder_vorbis_transform_pairs(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept {
    if (!valid_plan(plan) || values.size() < plan.half_block ||
        plan.half_block < 2) return false;
    for (std::uint32_t pair = 0; pair < plan.half_block / 2; ++pair) {
        const auto reversed = vorbis_transform_bit_reversal_index(plan, pair);
        if (!reversed || *reversed + 1 >= plan.half_block) return false;
        const auto right = plan.half_block - 2 - pair * 2;
        if (*reversed < right) {
            std::swap(values[*reversed], values[right]);
            std::swap(values[*reversed + 1], values[right + 1]);
        }
    }
    return true;
}

bool apply_vorbis_inverse_transform_rotation(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept {
    if (!valid_plan(plan) || values.size() < plan.half_block) return false;
    const auto half = static_cast<std::size_t>(plan.half_block);
    const auto stride = static_cast<std::size_t>(plan.table_stride);
    const auto& table = kVorbisTransformTableA;
    const auto wrap = [](std::int64_t value) noexcept {
        return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
    };
    const auto scaled = [](std::int32_t value) noexcept {
        return static_cast<std::int64_t>(value >> 8);
    };

    auto x = values[half - 3];
    auto y = values[half - 1];
    values[half - 3] = wrap(scaled(y) * table[1] + scaled(x) * table[0]);
    values[half - 1] = wrap(scaled(y) * table[0] - scaled(x) * table[1]);

    const auto first_count = (half / 2 - 3) / 4;
    std::size_t first_index = half - 7;
    std::size_t coefficient = stride;
    for (std::size_t i = 0; i < first_count; ++i) {
        x = values[first_index];
        y = values[first_index + 2];
        const auto c0 = table[coefficient];
        const auto c1 = table[coefficient + 1];
        values[first_index] = wrap(scaled(y) * c1 + scaled(x) * c0);
        values[first_index + 2] = wrap(scaled(y) * c0 - scaled(x) * c1);
        first_index -= 4;
        coefficient += stride;
    }

    const auto middle = half - 7 - 4 * first_count;
    if (middle + 2 >= half || (first_count + 1) * stride + 1 >= table.size()) {
        return false;
    }
    x = values[middle];
    y = values[middle + 2];
    coefficient = (first_count + 1) * stride;
    values[middle + 2] = wrap(scaled(y) * table[coefficient + 1] -
                              scaled(x) * table[coefficient]);
    values[middle] = wrap(scaled(y) * table[coefficient] +
                          scaled(x) * table[coefficient + 1]);

    auto second_index = middle - 4;
    auto second_count = middle / 4;
    auto reverse_coefficient = first_count * stride + 1;
    for (std::size_t i = 0; i < second_count; ++i) {
        if (second_index + 2 >= half || reverse_coefficient >= table.size()) return false;
        x = values[second_index];
        y = values[second_index + 2];
        const auto c1 = table[reverse_coefficient];
        const auto c0 = table[reverse_coefficient - 1];
        values[second_index] = wrap(scaled(y) * c0 + scaled(x) * c1);
        values[second_index + 2] = wrap(scaled(y) * c1 - scaled(x) * c0);
        second_index -= 4;
        reverse_coefficient -= stride;
    }

    x = values[0];
    y = values[2];
    const auto end_x = values[half - 4];
    const auto end_y = values[half - 2];
    values[half - 4] = wrap(scaled(y) * table[1] - scaled(x) * table[0]);
    values[half - 2] = wrap(scaled(x) * table[0] + scaled(y) * table[1]);
    values[0] = wrap(scaled(end_y) * table[stride] -
                     scaled(end_x) * table[stride + 1]);
    values[2] = wrap(scaled(end_y) * table[stride + 1] +
                     scaled(end_x) * table[stride]);

    const auto final_count = half / 8 - 1;
    auto right_index = half - 8;
    auto left_index = std::size_t{6};
    auto forward_coefficient = stride + 1;
    auto paired_coefficient = stride;
    for (std::size_t i = 0; i < final_count; ++i) {
        if (right_index + 2 >= half || left_index + 2 >= half ||
            forward_coefficient >= table.size() ||
            paired_coefficient + stride + 1 >= table.size()) {
            return false;
        }
        const auto left = values[left_index];
        const auto left_before = values[left_index - 2];
        const auto right = values[right_index];
        const auto right_after = values[right_index + 2];
        const auto c0 = table[forward_coefficient - 1];
        const auto c1 = table[forward_coefficient];
        values[right_index] = wrap(scaled(left) * c1 - scaled(left_before) * c0);
        values[right_index + 2] = wrap(scaled(left) * c0 + scaled(left_before) * c1);

        const auto next0 = table[paired_coefficient + stride];
        const auto next1 = table[paired_coefficient + stride + 1];
        values[left_index - 2] = wrap(scaled(right_after) * next0 - scaled(right) * next1);
        values[left_index] = wrap(scaled(right_after) * next1 + scaled(right) * next0);

        right_index -= 4;
        left_index += 4;
        forward_coefficient += stride;
        paired_coefficient += stride;
    }
    return true;
}

bool apply_vorbis_inverse_transform_cleanup(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept {
    if (!valid_plan(plan) || values.size() < plan.half_block ||
        plan.half_block % 32 != 0) {
        return false;
    }
    for (std::size_t offset = 0; offset < plan.half_block; offset += 32) {
        cleanup32(std::span<std::int32_t, 32>(values.data() + offset, 32));
    }
    return true;
}

bool apply_vorbis_inverse_transform_fft(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept {
    if (!valid_plan(plan) || values.size() < plan.half_block) return false;
    const auto& table = kVorbisTransformTableA;
    const auto half = static_cast<std::size_t>(plan.half_block);
    const auto stages = static_cast<std::uint8_t>(7 - plan.transform_shift);
    for (std::uint8_t stage = 0; stage < stages; ++stage) {
        const auto groups = std::size_t{1} << stage;
        const auto group_span = half >> stage;
        const auto coefficient_stride = std::size_t{4} <<
            (plan.transform_shift + stage);
        if (coefficient_stride == 0 || 0x400 % coefficient_stride != 0) return false;
        const auto coefficient_count = 0x400 / coefficient_stride;
        for (std::size_t group = 0; group < groups; ++group) {
            auto high = group * group_span + group_span - 4;
            auto low = group * group_span + group_span / 2 - 4;
            for (std::size_t i = 0; i < coefficient_count; ++i) {
                const auto coefficient = i * coefficient_stride;
                if (high + 3 >= half || low + 3 >= half ||
                    coefficient + 1 >= table.size()) {
                    return false;
                }
                butterfly_forward(values, high, low,
                                  table[coefficient], table[coefficient + 1]);
                high -= 4;
                low -= 4;
            }
            for (std::size_t i = 0; i < coefficient_count; ++i) {
                const auto coefficient = 0x400 - i * coefficient_stride;
                if (high + 3 >= half || low + 3 >= half ||
                    coefficient + 1 >= table.size()) {
                    return false;
                }
                butterfly_reverse(values, high, low,
                                  table[coefficient], table[coefficient + 1]);
                high -= 4;
                low -= 4;
            }
        }
    }
    return apply_vorbis_inverse_transform_cleanup(plan, values);
}

bool apply_vorbis_inverse_transform_recombination(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept {
    if (!valid_plan(plan) || values.size() < plan.half_block) return false;
    const auto half = static_cast<std::size_t>(plan.half_block);
    const auto stride = static_cast<std::size_t>(plan.table_stride);
    const auto base = stride > 3 ? stride / 2 : std::size_t{0x402};
    const auto table_at = [](std::size_t index) noexcept -> std::uint8_t {
        if (index < kVorbisTransformTableA.size()) {
            return kVorbisTransformTableA[index];
        }
        const auto second_index = index - 0x400;
        if (second_index < kVorbisTransformTableB.size()) {
            return kVorbisTransformTableB[second_index];
        }
        return 0;
    };
    const auto shifted = [](std::int32_t value, unsigned amount) noexcept {
        return value >> amount;
    };

    std::size_t left = 0;
    std::size_t right = half - 2;
    auto coefficient = base;
    while (coefficient - base < 0x400) {
        if (left + 1 >= half || right + 1 >= half || coefficient + 1 >= 0x802) {
            return false;
        }
        const auto x0 = values[left];
        const auto x1 = values[left + 1];
        const auto y0 = values[right];
        const auto y1 = values[right + 1];
        const auto c0 = table_at(coefficient);
        const auto c1 = table_at(coefficient + 1);
        const auto sum = shifted(add32(x0, y0), 9);
        const auto difference = shifted(sub32(y1, x1), 9);
        const auto rotation0 = add32(mul32(difference, c0), mul32(sum, c1));
        const auto rotation1 = sub32(mul32(difference, c1), mul32(sum, c0));
        const auto average0 = shifted(add32(x1, y1), 1);
        const auto average1 = shifted(sub32(x0, y0), 1);
        values[left] = add32(average0, rotation0);
        values[left + 1] = add32(average1, rotation1);
        values[right] = sub32(average0, rotation0);
        values[right + 1] = sub32(rotation1, average1);
        left += 2;
        right -= 2;
        coefficient += stride;
    }

    coefficient -= stride;
    do {
        if (right < 2 || left + 1 >= half || coefficient + 1 >= 0x802) {
            return false;
        }
        right -= 2;
        const auto x0 = values[left];
        const auto x1 = values[left + 1];
        const auto y0 = values[right];
        const auto y1 = values[right + 1];
        const auto c0 = table_at(coefficient);
        const auto c1 = table_at(coefficient + 1);
        const auto difference = shifted(sub32(y1, x1), 9);
        const auto sum = shifted(add32(x0, y0), 9);
        const auto rotation0 = add32(mul32(difference, c1), mul32(sum, c0));
        const auto rotation1 = sub32(mul32(difference, c0), mul32(sum, c1));
        const auto average0 = shifted(add32(x1, y1), 1);
        const auto average1 = shifted(sub32(x0, y0), 1);
        values[left] = add32(average0, rotation0);
        values[left + 1] = add32(average1, rotation1);
        values[right] = sub32(average0, rotation0);
        values[right + 1] = sub32(rotation1, average1);
        left += 2;
        coefficient -= stride;
    } while (left < right);
    return true;
}

bool apply_vorbis_inverse_transform_final_rotation(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept {
    if (!valid_plan(plan) || values.size() < plan.half_block) {
        return false;
    }
    const auto half = static_cast<std::size_t>(plan.half_block);
    if (plan.table_stride == 2) {
        if (plan.block_size != 8192 || half != 4096) return false;
        const auto a = [](std::size_t index) noexcept -> std::uint8_t {
            return index < kVorbisTransformTableA.size()
                ? kVorbisTransformTableA[index] : 0;
        };
        const auto b = [](std::size_t index) noexcept -> std::uint8_t {
            return index < kVorbisTransformTableB.size()
                ? kVorbisTransformTableB[index] : 0;
        };
        const auto q1 = [](std::uint8_t left, std::uint8_t right) noexcept {
            const auto delta = static_cast<std::int32_t>(right) - left;
            return static_cast<std::uint32_t>(
                static_cast<std::int32_t>(left) + (delta >> 2));
        };
        const auto q3 = [](std::uint8_t left, std::uint8_t right) noexcept {
            const auto delta = static_cast<std::int32_t>(right) - left;
            return static_cast<std::uint32_t>(
                static_cast<std::int32_t>(right) - (delta >> 2));
        };
        const auto rotate = [&](std::size_t index, std::uint32_t c0,
                                std::uint32_t c1) noexcept {
            const auto x = values[index] >> 8;
            const auto neg_y = sub32(0, values[index + 1]) >> 8;
            values[index] = add32(mul32(neg_y, static_cast<std::int32_t>(c1)),
                                  mul32(x, static_cast<std::int32_t>(c0)));
            values[index + 1] = sub32(mul32(neg_y, static_cast<std::int32_t>(c0)),
                                      mul32(x, static_cast<std::int32_t>(c1)));
        };

        rotate(0, q1(a(0), b(2)), q1(a(1), b(3)));
        rotate(2, q3(a(0), b(2)), q3(a(1), b(3)));
        rotate(4, q1(b(2), a(2)), q1(b(3), a(3)));
        rotate(6, q3(b(2), a(2)), q3(b(3), a(3)));

        for (std::size_t group = 0; group < half / 8 - 1; ++group) {
            const auto ai = group * 2 + 2;
            const auto bi = group * 2 + 4;
            const auto ac0 = a(ai);
            const auto ac1 = a(ai + 1);
            const auto bc0 = b(bi);
            const auto bc1 = b(bi + 1);
            const auto output = group * 8 + 8;
            rotate(output, q1(ac0, bc0), q1(ac1, bc1));
            rotate(output + 2, q3(ac0, bc0), q3(ac1, bc1));
            rotate(output + 4, q3(ac0, bc0), q3(ac1, bc1));
            rotate(output + 6, q1(ac0, bc0), q1(ac1, bc1));
        }
        return true;
    }
    if (plan.table_stride == 4) {
        const auto table_a = [](std::size_t index) noexcept -> std::uint8_t {
            return index < kVorbisTransformTableA.size()
                ? kVorbisTransformTableA[index] : 0;
        };
        const auto table_b = [](std::size_t index) noexcept -> std::uint8_t {
            if (index < kVorbisTransformTableB.size()) {
                return kVorbisTransformTableB[index];
            }
            const auto tail = index - kVorbisTransformTableB.size();
            return tail < kVorbisBitPermutationTable.size()
                ? kVorbisBitPermutationTable[tail] : 0;
        };
        if (plan.block_size != 4096 || half != 2048) return false;

        const auto rotate = [&](std::size_t index, std::uint32_t c0,
                                std::uint32_t c1) noexcept {
            const auto x = values[index] >> 8;
            const auto neg_y = sub32(0, values[index + 1]) >> 8;
            values[index] = add32(mul32(neg_y, static_cast<std::int32_t>(c1)),
                                  mul32(x, static_cast<std::int32_t>(c0)));
            values[index + 1] = sub32(mul32(neg_y, static_cast<std::int32_t>(c0)),
                                      mul32(x, static_cast<std::int32_t>(c1)));
        };
        const auto half_sum = [](std::uint32_t left, std::uint32_t right) noexcept {
            return (left >> 1) + (right >> 1);
        };

        const auto start0 = half_sum(table_b(2), table_a(0));
        const auto start1 = half_sum(table_b(3), table_a(1));
        rotate(0, start0, start1);
        rotate(2, half_sum(table_b(2), table_a(2)),
               half_sum(table_b(3), table_a(3)));

        for (std::size_t group = 0; group < half / 4 - 1; ++group) {
            const auto a = group * 2 + 2;
            const auto b = group * 2 + 4;
            rotate(group * 4 + 4, half_sum(table_a(a), table_b(b)),
                   half_sum(table_a(a + 1), table_b(b + 1)));
            rotate(group * 4 + 6, half_sum(table_a(a + 2), table_b(b)),
                   half_sum(table_a(a + 3), table_b(b + 1)));
        }
        return true;
    }
    if (plan.table_stride < 8) return false;
    const auto table_step = static_cast<std::size_t>(plan.table_stride / 4);
    const auto base = plan.table_stride > 12
        ? static_cast<std::size_t>(plan.table_stride / 8)
        : std::size_t{0x402};
    const auto table_at = [](std::size_t index) noexcept -> std::uint8_t {
        if (index < kVorbisTransformTableA.size()) {
            return kVorbisTransformTableA[index];
        }
        const auto second_index = index - 0x400;
        if (second_index < kVorbisTransformTableB.size()) {
            return kVorbisTransformTableB[second_index];
        }
        return 0;
    };

    const auto c0 = table_at(base);
    const auto c1 = table_at(base + 1);
    const auto x0 = values[0] >> 8;
    const auto neg_y0 = sub32(0, values[1]) >> 8;
    values[0] = add32(mul32(neg_y0, c1), mul32(x0, c0));
    values[1] = sub32(mul32(neg_y0, c0), mul32(x0, c1));

    const auto count = (half - 1) / 2;
    std::size_t coefficient = base;
    for (std::size_t pair = 1; pair <= count; ++pair) {
        const auto index = pair * 2;
        coefficient += table_step;
        if (index + 1 >= half || coefficient + 1 >= 0x802) return false;
        const auto value = values[index];
        const auto neg_next = sub32(0, values[index + 1]) >> 8;
        const auto c_left = table_at(coefficient);
        const auto c_right = table_at(coefficient + 1);
        values[index] = add32(mul32(neg_next, c_right),
                              mul32(value >> 8, c_left));
        values[index + 1] = sub32(mul32(neg_next, c_left),
                                  mul32(value >> 8, c_right));
    }
    return true;
}

bool apply_vorbis_inverse_transform_stages(
    const VorbisInverseTransformPlan& plan,
    std::span<std::int32_t> values) noexcept {
    if (!valid_plan(plan) || plan.block_size > 8192 ||
        values.size() < plan.half_block) {
        return false;
    }
    return apply_vorbis_inverse_transform_rotation(plan, values) &&
        apply_vorbis_inverse_transform_fft(plan, values) &&
        reorder_vorbis_transform_pairs(plan, values) &&
        apply_vorbis_inverse_transform_recombination(plan, values) &&
        apply_vorbis_inverse_transform_final_rotation(plan, values);
}

} // namespace echo::audio
