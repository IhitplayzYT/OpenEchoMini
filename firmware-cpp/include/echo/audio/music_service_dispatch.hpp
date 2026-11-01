#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::audio {

enum class MusicDispatchEffect : std::uint8_t {
    call,
    call_with_message_slice,
    call_after_nonzero_result,
    write_context_u32,
    write_indirect_u16,
    write_indirect_u32,
    clear_indirect_u8,
    clear_indirect_u32,
    copy_message_bytes,
    copy_global_byte,
    copy_global_u32,
    copy_global_byte_to_indirect,
    poll_event_with_limit,
    clear_event,
    notify_event,
    delay,
};

struct MusicDispatchStep {
    MusicDispatchEffect effect{};
    // Effect-specific function/global address or event ID.
    std::uint32_t target{};
    // Call: first scalar argument. Slice/copy: byte offset. Write: field offset.
    // call_after_nonzero_result: context pointer-field offset.
    std::uint32_t offset{};
    // Call: second scalar argument. Copy: byte count. Write: stored value.
    std::uint32_t value{};
    // Secondary call argument, source address/offset, or poll bound.
    std::uint32_t auxiliary{};
};

// Snapshot values needed to resolve branches around the switch. Names describe
// their use at the call site; their owning firmware structures are unknown.
struct MusicDispatchContext {
    std::uint32_t item_upper_bound{};
    std::uint32_t current_item_index{};
    std::uint32_t selection_state{};
    std::uint16_t service_state{};
    std::int32_t context_value_at_0x4c{};
    std::int8_t callback_argument{};
};

struct MusicDispatchPlan {
    std::uint16_t message_id{};
    bool handled{};
    std::uint32_t effective_argument{};
    std::array<MusicDispatchStep, 12> steps{};
    std::size_t step_count{};
};

// Decodes module 33's observed sparse service-message switch into ordered
// effects. A runtime adapter supplies the underlying shared-state and hardware
// operations. For poll_event_with_limit, the following delay step is repeated
// after poll result 1 until the event changes or the bound expires. Unresolved
// message names and payload fields remain raw offsets. Message-slice effects
// resolve the message pointer through the context field at offset 0x0c.
[[nodiscard]] MusicDispatchPlan plan_music_service_message(
    std::uint16_t message_id,
    std::uint32_t argument,
    const MusicDispatchContext& context) noexcept;

} // namespace echo::audio
