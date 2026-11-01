#include "echo/audio/module37_filter.hpp"

#include <bit>
#include <limits>

namespace echo::audio {
namespace {
std::uint64_t arithmetic_shift_right_10(std::uint64_t bits) {
    constexpr std::uint64_t sign = std::uint64_t{1} << 63;
    return (bits >> 10) | ((bits & sign) ? (~std::uint64_t{0} << 54) : 0);
}

std::uint64_t arithmetic_shift_right_13(std::uint64_t bits) {
    constexpr std::uint64_t sign = std::uint64_t{1} << 63;
    return (bits >> 13) | ((bits & sign) ? (~std::uint64_t{0} << 51) : 0);
}

std::uint64_t signed_product(std::int32_t left, std::int32_t right) {
    const auto product = static_cast<std::int64_t>(left) * right;
    return static_cast<std::uint64_t>(product);
}

std::uint64_t multiply_bits_by_signed(std::uint64_t bits, std::int32_t value) {
    // Unsigned multiplication gives the ARM low-64-bit result without signed
    // overflow undefined behavior.
    return bits * static_cast<std::uint64_t>(static_cast<std::int64_t>(value));
}

std::int32_t saturated_output(std::uint64_t q10_value) {
    const auto high = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(q10_value >> 32));
    const auto sign = high < 0 ? -1 : 0;
    if (sign != (high >> 1)) {
        return sign == 0 ? 0x007fffff :
                           std::bit_cast<std::int32_t>(0xff800000u);
    }
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(arithmetic_shift_right_10(q10_value)));
}

std::int32_t saturate_signed_23(std::int32_t value) {
    if (value > 0x003fffff) return 0x003fffff;
    if (value < -0x00400000) return -0x00400000;
    return value;
}

std::int16_t read_i16_le(std::span<const std::uint8_t> bytes,
                         std::size_t offset) {
    const auto bits = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(bytes[offset]) |
        (static_cast<std::uint16_t>(bytes[offset + 1]) << 8));
    return std::bit_cast<std::int16_t>(bits);
}

std::int32_t read_i32_le(std::span<const std::uint8_t> bytes,
                         std::size_t offset) {
    const auto bits = static_cast<std::uint32_t>(bytes[offset]) |
                      (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
                      (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
                      (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
    return std::bit_cast<std::int32_t>(bits);
}

bool process_filter_core(std::span<std::int32_t> words,
                         std::int32_t sample_count,
                         const Module37FilterCoefficients& c,
                         Module37FilterHistory& history,
                         bool short_counter) {
    if (short_counter && sample_count > std::numeric_limits<std::int16_t>::max()) {
        return false;
    }
    if (sample_count > 0 &&
        (words.empty() || static_cast<std::size_t>(sample_count - 1) >
                              (words.size() - 1u) / 2u)) return false;

    auto state0 = history.slot0;
    auto state1 = history.slot1;
    for (std::int32_t i = 0; i < sample_count; ++i) {
        const auto sample = words[static_cast<std::size_t>(i) * 2u];
        const auto accumulator = state0 + signed_product(sample, c.at_0x148);
        const auto q10 = arithmetic_shift_right_10(accumulator);
        const auto feedback0 = multiply_bits_by_signed(q10, c.at_0x140);
        const auto feedback1 = multiply_bits_by_signed(q10, c.at_0x144);
        const auto next0 = signed_product(sample, c.at_0x14c) + state1 +
                           arithmetic_shift_right_10(feedback0);
        const auto next1 = signed_product(sample, c.at_0x150) +
                           arithmetic_shift_right_10(feedback1);
        words[static_cast<std::size_t>(i) * 2u] = saturated_output(q10);
        state0 = next0;
        state1 = next1;
    }
    history.slot0 = state0;
    history.slot1 = state1;
    return true;
}
} // namespace

bool decode_module37_filter_coefficients(
    std::span<const std::uint8_t> module36_data, std::size_t slot,
    Module37FilterCoefficients& coefficients) {
    if (slot >= 10) return false;
    const std::size_t offset = 0x140u + slot * 0x14u;
    if (offset + 0x14u > module36_data.size()) return false;
    coefficients = {read_i32_le(module36_data, offset + 0x00u),
                    read_i32_le(module36_data, offset + 0x04u),
                    read_i32_le(module36_data, offset + 0x08u),
                    read_i32_le(module36_data, offset + 0x0cu),
                    read_i32_le(module36_data, offset + 0x10u)};
    return true;
}

bool module37_filter_history_offset(std::size_t slot, std::size_t bank_index,
                                    std::size_t& byte_offset) {
    if (slot >= 10) return false;
    constexpr std::size_t bank_stride = 10u * 0x10u;
    if (bank_index > (std::numeric_limits<std::size_t>::max() - slot * 0x10u) /
                         bank_stride) return false;
    byte_offset = (slot + bank_index * 10u) * 0x10u;
    return true;
}

bool process_module37_bank_b(std::span<std::int32_t> words,
                             std::int32_t sample_count,
                             const Module37FilterCoefficients& coefficients,
                             Module37FilterHistory& history) {
    if (sample_count <= 0) return false;
    return process_filter_core(words, sample_count, coefficients, history, false);
}

bool decode_module37_bank_a_coefficients(
    std::span<const std::uint8_t> module36_data, std::size_t slot,
    Module37BankACoefficients& coefficients) {
    if (slot >= 21) return false;
    const std::size_t first = 0x20u + 0x2a0u + slot * 4u;
    const std::size_t second = 0x20u + 0x2f4u + slot * 4u;
    if (first + 4u > module36_data.size() ||
        second + 4u > module36_data.size()) return false;
    coefficients = {read_i16_le(module36_data, first),
                    read_i16_le(module36_data, first + 2u),
                    read_i16_le(module36_data, second),
                    read_i16_le(module36_data, second + 2u)};
    return true;
}

bool module37_bank_a_history_offset(std::size_t slot, std::size_t bank_index,
                                   std::size_t& byte_offset) {
    if (slot >= 21) return false;
    constexpr std::size_t bank_stride = 0x2au * 8u;
    if (bank_index > (std::numeric_limits<std::size_t>::max() - 0x20u - slot * 16u) /
                         bank_stride) return false;
    byte_offset = 0x20u + (slot * 2u + bank_index * 0x2au) * 8u;
    return true;
}

bool process_module37_bank_a(std::span<std::int32_t> words,
                             std::int32_t sample_count,
                             const Module37BankACoefficients& c,
                             Module37FilterHistory& history) {
    if (sample_count <= 0) return false;
    const auto count = static_cast<std::size_t>(sample_count);
    if (count > std::numeric_limits<std::size_t>::max() - 7u) return false;
    const std::size_t rounded_count = ((count + 7u) / 8u) * 8u;
    if ((rounded_count - 1u) > (std::numeric_limits<std::size_t>::max() - 1u) / 2u ||
        words.size() <= (rounded_count - 1u) * 2u) return false;

    for (std::size_t block = 0; block < rounded_count; block += 8u) {
        std::int32_t input[8];
        std::int32_t q[8];
        for (std::size_t i = 0; i < 8u; ++i) {
            input[i] = words[(block + i) * 2u];
        }
        auto shifted = arithmetic_shift_right_13(
            history.slot0 + signed_product(input[0], 0x2000));
        q[0] = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(shifted));
        words[block * 2u] = saturate_signed_23(q[0]);

        shifted = arithmetic_shift_right_13(
            history.slot1 + signed_product(q[0], c.feedback1) +
            signed_product(input[0], c.feedforward1) +
            signed_product(input[1], 0x2000));
        q[1] = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(shifted));
        words[(block + 1u) * 2u] = saturate_signed_23(q[1]);

        for (std::size_t i = 2; i < 8u; ++i) {
            const auto accumulator = signed_product(input[i], 0x2000) +
                signed_product(q[i - 2u], c.feedback2) +
                signed_product(input[i - 2u], c.feedforward2) +
                signed_product(q[i - 1u], c.feedback1) +
                signed_product(input[i - 1u], c.feedforward1);
            shifted = arithmetic_shift_right_13(accumulator);
            q[i] = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(shifted));
            words[(block + i) * 2u] = saturate_signed_23(q[i]);
        }

        history.slot0 = signed_product(q[6], c.feedback2) +
                        signed_product(input[6], c.feedforward2) +
                        signed_product(q[7], c.feedback1) +
                        signed_product(input[7], c.feedforward1);
        history.slot1 = signed_product(q[7], c.feedback2) +
                        signed_product(input[7], c.feedforward2);
    }
    return true;
}

bool process_module37_filter(std::span<std::int32_t> words,
                             std::int32_t sample_count,
                             const Module37FilterCoefficients& c,
                             Module37FilterHistory& history) {
    return process_filter_core(words, sample_count, c, history, true);
}

std::vector<Module37BankCall> plan_module37_primary_bank(
    std::span<const std::uint8_t> module36_data, std::uintptr_t parameter1,
    std::uintptr_t parameter2, std::uintptr_t shared_table,
    std::uintptr_t parameter3) {
    std::vector<Module37BankCall> calls;
    if (module36_data.size() < 31) return calls;
    for (int slot = 20; slot >= 0; --slot) {
        if (module36_data[static_cast<std::size_t>(slot) + 10u] != 0) continue;
        calls.push_back({static_cast<std::uint8_t>(slot), 5,
                         {parameter2, parameter1, shared_table,
                          static_cast<std::uintptr_t>(slot * 2), parameter3}});
    }
    return calls;
}

std::vector<Module37BankCall> plan_module37_secondary_bank(
    std::span<const std::uint8_t> module36_data, std::uintptr_t parameter1,
    std::uintptr_t parameter2, std::uintptr_t parameter3) {
    std::vector<Module37BankCall> calls;
    if (module36_data.size() < 10) return calls;
    for (int slot = 9; slot >= 0; --slot) {
        if (module36_data[static_cast<std::size_t>(slot)] != 0) continue;
        calls.push_back({static_cast<std::uint8_t>(slot), 4,
                         {parameter1, parameter2,
                          static_cast<std::uintptr_t>(slot), parameter3, 0}});
    }
    return calls;
}

} // namespace echo::audio
