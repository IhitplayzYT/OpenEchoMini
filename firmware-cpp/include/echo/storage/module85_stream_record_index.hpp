#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace echo::storage {

inline constexpr std::uint32_t kModule85FixedRecordBytes = 0x4c;
inline constexpr std::size_t kModule85RecordKeyOffset = 0;
inline constexpr char kModule85IndexObjectName[] = "WOOOOONXBIN";
inline constexpr char kModule85PayloadOutputMode[] = "W";
inline constexpr std::size_t kModule85EntryHeaderBytes = 0x24;

struct Module85EntryHeader {
    // The entry reads nine little-endian words. Words 4, 5, and 6 are copied
    // to shared state; the other word meanings are unresolved.
    std::array<std::uint32_t, 9> words{};
};

[[nodiscard]] std::optional<Module85EntryHeader>
decode_module85_entry_header(std::span<const std::uint8_t> bytes) noexcept;

struct Module85EntryPlan {
    bool cleanup_previous_object{};
    bool continue_after_cleanup{true};
    bool close_new_object_without_read{};
};

// Models only the observed parameter gates in module 85's entry routine.
[[nodiscard]] constexpr Module85EntryPlan
plan_module85_entry(std::uint32_t mode) noexcept {
    return {mode < 3U, mode != 2U, mode == 6U};
}

enum class Module85LookupGate {
    no_descriptor,
    mode_passthrough,
    open_and_search,
};

struct Module85LookupGatePlan {
    Module85LookupGate gate{Module85LookupGate::no_descriptor};
    // FUN_0307b7c2 returns 1 when no descriptor is present, or returns the
    // shared mode byte when it is not 1. Ignored for open_and_search.
    std::uint8_t immediate_result{};
    // The observed caller passes null for FUN_0307b56e's payload-output input.
    bool payload_output_requested{};
};

[[nodiscard]] constexpr Module85LookupGatePlan plan_module85_lookup_gate(
    const std::uint8_t descriptor_state,
    const std::uint8_t shared_mode) noexcept {
    if (descriptor_state == 0U) {
        return {Module85LookupGate::no_descriptor, 1U, false};
    }
    if (shared_mode != 1U) {
        return {Module85LookupGate::mode_passthrough, shared_mode, false};
    }
    return {Module85LookupGate::open_and_search, 0U, false};
}

struct Module85StreamRecordTable {
    std::uint8_t stream_id{};
    std::uint32_t record_count{};
    std::uint32_t record_base_offset{};
};

struct Module85SearchBounds {
    std::uint32_t lower{};
    std::uint32_t upper{};
};

struct Module85RecordReadPlan {
    std::uint32_t index{};
    std::uint32_t byte_offset{};
    std::uint32_t byte_count{kModule85FixedRecordBytes};
    std::uint8_t stream_id{};
};

enum class Module85SearchUpdate {
    invalid,
    continue_search,
    found,
};

// Plans the seek/read at the midpoint used by FUN_0307b56e. Offset arithmetic
// preserves the firmware's 32-bit wrap; no stream I/O is performed here.
[[nodiscard]] std::optional<Module85RecordReadPlan>
plan_module85_record_probe(const Module85StreamRecordTable& table,
                           Module85SearchBounds bounds) noexcept;

// compare_order is key-versus-record: positive advances lower, negative
// reduces upper, and zero reports a match.
[[nodiscard]] Module85SearchUpdate update_module85_search_bounds(
    Module85SearchBounds& bounds, std::uint32_t probe_index,
    int compare_order) noexcept;

// The shared Thumb helper at runtime 0x03055d50 compares two NUL-terminated
// byte strings using unsigned byte values. Returns no value when the supplied
// spans end before a mismatch or shared terminator, rather than reading past
// either bound as the firmware can.
[[nodiscard]] std::optional<int> compare_module85_zero_terminated_key(
    std::span<const std::uint8_t> query,
    std::span<const std::uint8_t> record) noexcept;

// FUN_0307b56e passes the start of each complete record to that comparator,
// establishing the key field at byte offset zero.
[[nodiscard]] std::optional<int> compare_module85_record_key(
    std::span<const std::uint8_t> query,
    std::span<const std::uint8_t, kModule85FixedRecordBytes> record) noexcept;

} // namespace echo::storage
