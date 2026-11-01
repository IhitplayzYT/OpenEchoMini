#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <optional>
#include <span>

namespace echo::audio {

enum class Module75AacConfigStatus : std::int8_t {
    ok = 0,
    input_error = -5,
    invalid_descriptor = -8,
};

struct Module75AacConfigIndices {
    std::uint8_t frequency_index{};
    std::uint8_t channel_configuration_index{};
    std::uint8_t source_config_byte_0{};
    std::uint8_t source_config_byte_1{};
};

// Parses the descriptor sequence consumed by FUN_01003c24. The function
// starts at the caller's current position; the firmware skips four bytes
// before examining the first descriptor tag.
Module75AacConfigStatus parse_module75_aac_config_descriptors(
    std::span<const std::uint8_t> bytes,
    Module75AacConfigIndices& output) noexcept;

struct Module75AacElementTag {
    bool is_channel_pair{};
    std::uint8_t tag_select{};
};

struct Module75AacCouplingTag {
    bool independent_switch{};
    std::uint8_t tag_select{};
};

struct Module75AacProgramConfig {
    std::uint8_t element_instance_tag{};
    std::uint8_t object_type{};
    std::uint8_t sampling_frequency_index{};
    std::uint8_t front_count{};
    std::uint8_t side_count{};
    std::uint8_t back_count{};
    std::uint8_t lfe_count{};
    std::uint8_t associated_data_count{};
    std::uint8_t coupling_channel_count{};
    bool mono_mixdown_present{};
    std::uint8_t mono_mixdown_element_number{};
    bool stereo_mixdown_present{};
    std::uint8_t stereo_mixdown_element_number{};
    bool matrix_mixdown_present{};
    std::uint8_t matrix_mixdown_index{};
    bool pseudo_surround_enable{};
    std::array<Module75AacElementTag, 15> front{};
    std::array<Module75AacElementTag, 15> side{};
    std::array<Module75AacElementTag, 15> back{};
    std::array<std::uint8_t, 3> lfe_tag_select{};
    std::array<std::uint8_t, 7> associated_data_tag_select{};
    std::array<Module75AacCouplingTag, 15> coupling_channel{};
    std::uint8_t comment_length{};
    std::size_t comment_offset{};
    std::size_t bytes_consumed{};
};

// Parses the PCE fields observed in FUN_01006ac0. The returned comment offset
// is relative to the beginning of the supplied span; comment bytes are skipped.
Module75AacConfigStatus parse_module75_aac_program_config(
    std::span<const std::uint8_t> bytes,
    Module75AacProgramConfig& output) noexcept;

struct Module75AacIcsInfo {
    bool reserved_bit{};
    std::uint8_t window_sequence{};
    bool window_shape{};
    std::uint8_t max_sfb{};
    bool predictor_data_present{};
    bool predictor_reset{};
    std::uint8_t predictor_reset_group{};
    std::array<bool, 64> predictor_used{};
    std::uint8_t predictor_flag_count{};
    std::uint8_t window_group_count{};
    std::array<std::uint8_t, 8> window_group_length{};
    std::size_t bits_consumed{};
};

// max_prediction_sfb is the sample-rate-indexed firmware table result used
// by FUN_01007a00; the relocated table itself remains caller-supplied.
Module75AacConfigStatus parse_module75_aac_ics_info(
    std::span<const std::uint8_t> bits,
    std::uint8_t max_prediction_sfb,
    Module75AacIcsInfo& output,
    std::size_t start_bit_offset = 0) noexcept;

struct Module75AacChannelPairHeader {
    std::uint8_t element_instance_tag{};
    bool common_window{};
    Module75AacIcsInfo ics_info{};
    std::uint8_t ms_mask_present{};
    std::array<bool, 8 * 64> ms_used_by_group_sfb{};
    std::size_t ms_mask_bit_count{};
    std::size_t bits_consumed{};
};

// Parses the CPE header after its 3-bit raw_data_block element ID. If a common
// window is present, the ICS info precedes the two-bit mid-side mask selector.
Module75AacConfigStatus parse_module75_aac_channel_pair_header(
    std::span<const std::uint8_t> bits,
    std::uint8_t max_prediction_sfb,
    Module75AacChannelPairHeader& output,
    std::size_t start_bit_offset = 0) noexcept;

struct Module75AacSingleElementHeader {
    std::uint8_t element_instance_tag{};
    bool low_frequency_effect{};
    std::size_t bits_consumed{};
};

// Parses the 4-bit tag used by SCE (element 0) and LFE (element 3) cases.
Module75AacConfigStatus parse_module75_aac_single_element_header(
    std::span<const std::uint8_t> bits,
    bool low_frequency_effect,
    Module75AacSingleElementHeader& output,
    std::size_t start_bit_offset = 0) noexcept;

struct Module75AacSection {
    std::uint8_t codebook{};
    std::uint8_t first_sfb{};
    std::uint8_t sfb_count{};
};

struct Module75AacSectionData {
    std::uint8_t window_group_count{};
    std::uint8_t max_sfb{};
    std::uint8_t section_length_bits{};
    std::array<std::uint8_t, 8 * 64> codebook_by_group_sfb{};
    std::array<std::uint8_t, 8> section_count{};
    std::array<std::array<Module75AacSection, 64>, 8> sections{};
    std::size_t bits_consumed{};
};

// Parses section codebooks from the cursor immediately after AAC global gain
// and ICS info, using window shape and limits from the decoded ICS record.
Module75AacConfigStatus parse_module75_aac_section_data(
    std::span<const std::uint8_t> bits,
    const Module75AacIcsInfo& ics,
    Module75AacSectionData& output) noexcept;

struct Module75AacHuffmanSymbol {
    std::int16_t value{};
    std::uint8_t bits_consumed{};
};

// A raw-data-block DSE/FIL payload remains in the caller's input buffer. The
// bit offset is absolute within that supplied buffer, so callers can expose or
// copy the payload without rebuilding the surrounding AAC element.
struct Module75AacPayloadSpan {
    std::uint8_t element_instance_tag{};
    bool byte_align{};
    std::uint16_t payload_length{};
    std::size_t payload_bit_offset{};
    std::size_t bits_consumed{};
};

struct Module75AacHuffmanTableView {
    std::span<const std::uint8_t> count_by_depth{};
    std::span<const std::int16_t> symbol_table{};
    std::size_t symbol_table_base{};
};

struct Module75AacScalefactors {
    std::uint8_t global_gain{};
    std::array<std::int32_t, 8 * 64> value_by_group_sfb{};
    std::size_t scalefactor_count{};
    std::size_t bits_consumed{};
};

struct Module75AacPulseData {
    bool present{};
    std::uint8_t pulse_count{};
    std::uint8_t pulse_start_sfb{};
    std::array<std::uint8_t, 4> offset{};
    std::array<std::uint8_t, 4> amplitude{};
    std::size_t bits_consumed{};
};

struct Module75AacTnsFilter {
    std::uint8_t length{};
    std::uint8_t order{};
    bool direction{};
    bool coefficient_compressed{};
    std::uint8_t coefficient_count{};
    std::array<std::uint8_t, 31> coefficients{};
};

struct Module75AacTnsData {
    bool present{};
    bool short_window{};
    std::uint8_t window_count{};
    std::array<std::uint8_t, 8> filter_count{};
    std::array<std::array<Module75AacTnsFilter, 3>, 8> filters{};
    std::size_t bits_consumed{};
};

struct Module75AacGainAdjustment {
    std::uint8_t level_code{};
    std::uint8_t location{};
};

struct Module75AacGainControlLayout {
    std::uint8_t windows_per_band{};
    std::uint8_t first_window_location_bits{};
    std::uint8_t later_window_location_bits{};
};

struct Module75AacGainControlData {
    bool present{};
    std::uint8_t max_band{};
    std::array<std::array<std::uint8_t, 8>, 4> adjustment_count{};
    std::array<std::array<std::array<Module75AacGainAdjustment, 7>, 8>, 4>
        adjustments{};
    std::size_t bits_consumed{};
};

Module75AacConfigStatus parse_module75_aac_gain_control_data(
    std::span<const std::uint8_t> bits,
    const Module75AacGainControlLayout& layout,
    Module75AacGainControlData& output) noexcept;

Module75AacConfigStatus parse_module75_aac_tns_data(
    std::span<const std::uint8_t> bits,
    bool short_window,
    Module75AacTnsData& output) noexcept;

Module75AacConfigStatus parse_module75_aac_pulse_data(
    std::span<const std::uint8_t> bits,
    Module75AacPulseData& output) noexcept;

Module75AacConfigStatus apply_module75_aac_pulse_corrections(
    std::span<std::int32_t> spectral_lines,
    std::span<const std::uint16_t> scalefactor_band_offsets,
    const Module75AacPulseData& pulse) noexcept;

// Reads global gain and the scalefactor/noise/intensity deltas after section
// data. Relocated Huffman tables are indexed by the section codebook and are
// supplied by the caller; codebooks 0 and 12 are represented as zero here.
Module75AacConfigStatus parse_module75_aac_scalefactors(
    std::span<const std::uint8_t> bits,
    const Module75AacSectionData& sections,
    std::span<const Module75AacHuffmanTableView, 16> tables_by_codebook,
    Module75AacScalefactors& output) noexcept;

// start_bit_after_element_id points just after the 3-bit AAC element ID.
// DSE's count 255 is followed by an 8-bit extension and totals 255 + ext.
Module75AacConfigStatus parse_module75_aac_data_stream_element(
    std::span<const std::uint8_t> bits,
    std::size_t start_bit_after_element_id,
    Module75AacPayloadSpan& output) noexcept;

// start_bit_after_element_id points just after the 3-bit AAC element ID.
// FIL's count 15 is followed by an 8-bit escape and totals 14 + escape.
Module75AacConfigStatus parse_module75_aac_fill_element(
    std::span<const std::uint8_t> bits,
    std::size_t start_bit_after_element_id,
    Module75AacPayloadSpan& output) noexcept;

// Ports the depth-count/table-index recurrence in FUN_01008efc. The supplied
// codeword is top-aligned, as produced by the firmware's 32-bit bit peek.
std::optional<Module75AacHuffmanSymbol> decode_module75_aac_huffman_symbol(
    std::uint32_t codeword,
    std::span<const std::uint8_t> count_by_depth,
    std::span<const std::int16_t> symbol_table,
    std::size_t symbol_table_base = 0) noexcept;

struct Module75AacSpectralVector {
    std::uint8_t codebook{};
    std::uint8_t dimension{};
    std::uint8_t sign_bits_consumed{};
    std::array<std::int32_t, 4> coefficients{};
    std::size_t bits_consumed{};
};

// Decodes one spectral tuple from AAC codebooks 1..11. The table row stores
// packed coefficient magnitudes/count metadata; the caller supplies exact
// relocated depth counts and symbol rows for this codebook.
Module75AacConfigStatus parse_module75_aac_spectral_vector(
    std::span<const std::uint8_t> bits,
    std::uint8_t codebook,
    const Module75AacHuffmanTableView& table,
    Module75AacSpectralVector& output,
    std::size_t start_bit_offset = 0) noexcept;

struct Module75AacSpectralBand {
    std::uint8_t codebook{};
    std::uint16_t line_count{};
    std::uint16_t vector_count{};
    std::array<std::int32_t, 1024> coefficients{};
    std::size_t bits_consumed{};
};

// Decodes one SFB-sized coefficient band. Codebooks outside 1..11 produce
// zeros as observed in the firmware's spectral dispatch; codebook 11 handles
// its escape values in the vector parser above.
Module75AacConfigStatus parse_module75_aac_spectral_band(
    std::span<const std::uint8_t> bits,
    std::uint8_t codebook,
    std::uint16_t line_count,
    const Module75AacHuffmanTableView& table,
    Module75AacSpectralBand& output) noexcept;

// FUN_01008bf8 clamps a signed accumulator to the Q15/int16 range and returns
// the low 16 bits when it already fits.
std::int16_t saturate_module75_aac_q15(std::int32_t value) noexcept;

// Ports the signed high-word product used by the fixed-point transform
// butterflies and overlap path (SMULL followed by an arithmetic >> 32).
std::int32_t multiply_high_module75_aac_q31(
    std::int32_t left,
    std::int32_t right) noexcept;

// FUN_010099de clears the remaining spectral tail in groups of four lines.
Module75AacConfigStatus zero_fill_module75_aac_spectrum_tail(
    std::span<std::int32_t> storage,
    std::size_t first_unused_line,
    std::size_t remaining_line_count) noexcept;

}  // namespace echo::audio
