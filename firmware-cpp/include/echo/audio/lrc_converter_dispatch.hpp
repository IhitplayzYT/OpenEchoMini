#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <optional>
#include <vector>

namespace echo::audio {

enum class LrcConverterEntry {
    address_030634ac,
    address_030632c0,
};

struct LrcConverterPlan {
    std::uint16_t encoding_id{};
    LrcConverterEntry entry{};
};

enum class LrcDecodeMode : std::uint8_t {
    firmware_legacy = 0,
    utf8 = 3,
};

enum class LrcDecodeStop : std::uint8_t {
    input_limit,
    nul_terminator,
    truncated_sequence,
    unsupported_sequence,
    output_limit,
};

struct LrcDecodeResult {
    std::vector<std::uint16_t> utf16_units;
    std::size_t bytes_consumed{};
    LrcDecodeStop stop{LrcDecodeStop::input_limit};
};

struct LrcCompatibilityRange {
    std::uint16_t first{};
    std::uint16_t last{};
    std::uint16_t output_first{};
};

// Module 27 dispatches conversion by the encoding ID stored in its sidecar
// context. A UTF-8 BOM selects the nonzero converter with mode 3; the zero-ID
// entry selects legacy mode. Other nonzero encoding IDs also use the UTF-8
// decoder entry, with their runtime mode/table selection still unresolved.
[[nodiscard]] LrcConverterPlan plan_lrc_converter(
    std::uint16_t encoding_id) noexcept;

// Models bounded ASCII plus two/three-byte UTF-8 sequences, and the observed
// legacy mapping path. UTF-8 high single bytes and 4–6-byte forms use runtime
// map/store behavior not yet integrated here. Legacy DBCS pairs use the
// selected extracted c8 map and separate secondary fallback table. Automatic
// compatibility-range remapping is also not yet modeled.
[[nodiscard]] LrcDecodeResult decode_lrc_bytes(
    std::span<const std::uint8_t> input,
    LrcDecodeMode mode,
    std::span<const std::uint8_t> codepage_region,
    std::span<const std::uint8_t> secondary_fallback_region,
    std::uint8_t codepage_index,
    std::size_t max_input_bytes,
    std::size_t max_output_units);

// Raw 54-entry remap table at module 0 offset 0x21cbc. Its placement in the
// converter's operation order is not yet proven, so callers opt into lookup.
[[nodiscard]] std::span<const LrcCompatibilityRange>
lrc_compatibility_ranges() noexcept;

[[nodiscard]] std::optional<std::uint16_t>
lookup_lrc_compatibility_codepoint(std::uint16_t input) noexcept;

// Reads the observed little-endian secondary fallback table for logical
// legacy pair values in A140..F9FE. Zero slots use the firmware's U+3000
// fallback; values outside the table range return nullopt.
[[nodiscard]] std::optional<std::uint16_t>
lookup_lrc_secondary_fallback(
    std::span<const std::uint8_t> table,
    std::uint16_t logical_pair) noexcept;

} // namespace echo::audio
