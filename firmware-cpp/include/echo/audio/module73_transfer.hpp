#pragma once

#include <cstdint>
#include <optional>

namespace echo::audio {

struct Module73TransferInput {
    std::uint32_t channel{};
    std::uint32_t destination_base{};
    std::uint32_t source_base{};
    std::uint32_t transfer_length{};
    std::uint32_t descriptor_base{};
    std::uint32_t flags{};
    std::uint32_t metadata_word1{};
    std::uint32_t metadata_word2{};
    std::uint32_t event_mask_low{};
    std::uint32_t event_mask_high{};
};

struct Module73TransferDescriptor {
    std::uint32_t destination{};
    std::uint32_t source{};
    std::uint32_t next{};
    std::uint32_t flags{};
    std::uint32_t length{};
};

struct Module73TransferPlan {
    std::uint32_t channel_record_offset{};
    std::uint32_t destination_base{};
    std::uint32_t source_base{};
    std::uint32_t metadata_word1{};
    std::uint32_t metadata_word2{};
    std::uint32_t caller_flags_after{};
    std::uint32_t control_flags{};
    std::uint32_t control_length{};
    std::uint32_t descriptor_pointer{};
    std::uint32_t descriptor_count{};
    std::uint32_t descriptor_base{};
    std::int32_t destination_stride{};
    std::int32_t source_stride{};
    std::uint32_t final_descriptor_length{};
    std::uint32_t event_mask_low_after{};
    std::uint32_t event_mask_high_after{};
    bool segmented{};
    bool length_arithmetic_wrapped{};
    bool event_mask_shift_out_of_range{};

    [[nodiscard]] std::optional<Module73TransferDescriptor> descriptor_at(
        std::uint32_t index) const noexcept;
};

// Models FUN_01000e10's channel record and optional 0xff8-byte descriptor chain.
[[nodiscard]] Module73TransferPlan plan_module73_transfer(
    const Module73TransferInput& input) noexcept;

} // namespace echo::audio
