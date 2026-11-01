#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace echo::audio {

struct Module37FilterCoefficients {
    // Coefficient word offsets from each 0x14-byte filter record.
    std::int32_t at_0x140{};
    std::int32_t at_0x144{};
    std::int32_t at_0x148{};
    std::int32_t at_0x14c{};
    std::int32_t at_0x150{};
};

struct Module37FilterHistory {
    // These are the two 64-bit accumulator slots addressed by the overlay.
    std::uint64_t slot0{};
    std::uint64_t slot1{};
};

struct Module37BankACoefficients {
    std::int16_t feedback1{};   // table + 0x2a0 + slot * 4
    std::int16_t feedback2{};   // table + 0x2a2 + slot * 4
    std::int16_t feedforward1{}; // table + 0x2f4 + slot * 4
    std::int16_t feedforward2{}; // table + 0x2f6 + slot * 4
};

bool decode_module37_filter_coefficients(
    std::span<const std::uint8_t> module36_data, std::size_t slot,
    Module37FilterCoefficients& coefficients);

bool module37_filter_history_offset(std::size_t slot, std::size_t bank_index,
                                    std::size_t& byte_offset);

// Reads the four signed halfword coefficients used by FUN_0308bfa8. The
// module36 data span begins at execution address 0x0304b04c.
bool decode_module37_bank_a_coefficients(
    std::span<const std::uint8_t> module36_data, std::size_t slot,
    Module37BankACoefficients& coefficients);

// Returns the first history byte offset in module36_data for a slot and bank
// index. The second consecutive 64-bit value is the other history slot.
bool module37_bank_a_history_offset(std::size_t slot, std::size_t bank_index,
                                   std::size_t& byte_offset);

// Applies FUN_0308c3f2 to words[0], words[2], ... in place. The intervening
// words are not read or written by this procedure. History is updated even for
// a zero or negative sample count, matching the no-loop path.
// Returns false for a span too short for the requested strided samples or for
// a count beyond the signed 16-bit loop counter's non-wrapping range.
bool process_module37_filter(std::span<std::int32_t> words,
                             std::int32_t sample_count,
                             const Module37FilterCoefficients& coefficients,
                             Module37FilterHistory& history);

bool process_module37_bank_b(std::span<std::int32_t> words,
                             std::int32_t sample_count,
                             const Module37FilterCoefficients& coefficients,
                             Module37FilterHistory& history);

struct Module37BankCall {
    std::uint8_t slot{};
    std::uint8_t argument_count{};
    std::uintptr_t arguments[5]{};
};

// Builds the descending slot-call order used by FUN_0308c37c. It reads the
// exact zero/nonzero byte tests at module36_data[slot + 10].
std::vector<Module37BankCall> plan_module37_primary_bank(
    std::span<const std::uint8_t> module36_data, std::uintptr_t parameter1,
    std::uintptr_t parameter2, std::uintptr_t shared_table,
    std::uintptr_t parameter3);

// Builds the descending slot-call order used by FUN_0308c4e8. It tests
// module36_data[slot]; zero means call.
std::vector<Module37BankCall> plan_module37_secondary_bank(
    std::span<const std::uint8_t> module36_data, std::uintptr_t parameter1,
    std::uintptr_t parameter2, std::uintptr_t parameter3);

// Reconstructs FUN_0308bfa8. It processes complete groups of eight samples,
// rounding a positive count up to the next group as the firmware's do/while
// loop does. ARM SSAT #23 clamps the intermediate result. Only words[2*i] are
// changed; words[2*i+1] are left untouched.
bool process_module37_bank_a(std::span<std::int32_t> words,
                             std::int32_t sample_count,
                             const Module37BankACoefficients& coefficients,
                             Module37FilterHistory& history);

} // namespace echo::audio
