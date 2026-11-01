#include "echo/audio/module73_transfer.hpp"

namespace echo::audio {

std::optional<Module73TransferDescriptor> Module73TransferPlan::descriptor_at(
    std::uint32_t index) const noexcept {
    if (!segmented || index >= descriptor_count) return std::nullopt;
    const auto destination_delta =
        static_cast<std::uint32_t>(destination_stride) * index;
    const auto source_delta = static_cast<std::uint32_t>(source_stride) * index;
    const std::uint32_t destination = destination_base + destination_delta;
    const std::uint32_t source = source_base + source_delta;
    const std::uint32_t next = index + 1 == descriptor_count
        ? 0u : descriptor_base + (index + 1) * 0x18u;
    const std::uint32_t length = index + 1 == descriptor_count
        ? final_descriptor_length : 0xff8u;
    return Module73TransferDescriptor{
        destination, source, next, caller_flags_after | 0x18000000u, length};
}

Module73TransferPlan plan_module73_transfer(
    const Module73TransferInput& input) noexcept {
    Module73TransferPlan plan{};
    plan.channel_record_offset = input.channel * 0x58u;
    plan.destination_base = input.destination_base;
    plan.source_base = input.source_base;
    plan.metadata_word1 = input.metadata_word1;
    plan.metadata_word2 = input.metadata_word2;
    plan.caller_flags_after = input.flags | 1u;
    plan.descriptor_base = input.descriptor_base;
    plan.event_mask_low_after = input.event_mask_low;
    plan.event_mask_high_after = input.event_mask_high;

    if (input.transfer_length <= 0xff8u) {
        plan.control_flags = plan.caller_flags_after;
        plan.control_length = input.transfer_length;
    } else {
        plan.segmented = true;
        plan.control_flags = plan.caller_flags_after | 0x18000000u;
        plan.descriptor_pointer = input.descriptor_base;
        const std::uint32_t rounded = input.transfer_length + 0xff7u;
        plan.length_arithmetic_wrapped = rounded < input.transfer_length;
        plan.descriptor_count = rounded / 0xff8u;
        const std::uint32_t consumed = plan.descriptor_count * 0xff8u;
        plan.final_descriptor_length =
            input.transfer_length - consumed + 0xff8u;

        const auto flags = plan.caller_flags_after;
        const auto destination_mode = flags & 0x600u;
        if (destination_mode == 0 || destination_mode == 0x200u) {
            const auto shift = (flags & 0x30u) >> 4;
            const auto stride = static_cast<std::int32_t>(0xff8u << shift);
            plan.destination_stride = destination_mode == 0 ? stride : -stride;
        }
        const auto source_mode = flags & 0x180u;
        if (source_mode == 0 || source_mode == 0x80u) {
            const auto shift = (flags & 0x6u) >> 1;
            const auto stride = static_cast<std::int32_t>(0xff8u << shift);
            plan.source_stride = source_mode == 0 ? stride : -stride;
        }
    }

    const std::uint32_t shift = input.channel & 0xffu;
    std::uint32_t mask_low = 0;
    if (shift < 32) mask_low = 0x101u << shift;
    else plan.event_mask_shift_out_of_range = true;
    const std::uint32_t mask_high =
        (mask_low & 0x80000000u) != 0 ? 0xffffffffu : 0u;
    plan.event_mask_low_after |= mask_low;
    plan.event_mask_high_after |= mask_high;
    return plan;
}

} // namespace echo::audio
