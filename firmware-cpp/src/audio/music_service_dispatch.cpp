#include "echo/audio/music_service_dispatch.hpp"

namespace echo::audio {
namespace {

constexpr std::uint32_t kContext = 0x03089dc0;
constexpr std::uint32_t kSelection = 0x03089dc8;
constexpr std::uint32_t kSharedState = 0x03089db8;
constexpr std::uint32_t kModeBytePointer = 0x03089dc4;
constexpr std::uint32_t kPayloadDestination = 0x03089dd8;

void push(MusicDispatchPlan& plan,
          const MusicDispatchEffect effect,
          const std::uint32_t target = 0,
          const std::uint32_t offset = 0,
          const std::uint32_t value = 0,
          const std::uint32_t auxiliary = 0) noexcept {
    if (plan.step_count < plan.steps.size()) {
        plan.steps[plan.step_count++] = {effect, target, offset, value, auxiliary};
    }
}

void call(MusicDispatchPlan& plan,
          const std::uint32_t address,
          const std::uint32_t first = 0,
          const std::uint32_t second = 0) noexcept {
    push(plan, MusicDispatchEffect::call, address, first, second);
}

} // namespace

MusicDispatchPlan plan_music_service_message(
    const std::uint16_t message_id,
    const std::uint32_t argument,
    const MusicDispatchContext& context) noexcept {
    MusicDispatchPlan plan{};
    plan.message_id = message_id;
    plan.effective_argument = argument;

    const auto handler = [&](const std::uint32_t address) {
        plan.handled = true;
        call(plan, address);
    };

    switch (message_id) {
    case 0xec:
        handler(0x03088736);
        break;
    case 0xed:
        handler(0x03088a98);
        break;
    case 0xee:
        handler(0x030893a8);
        break;
    case 0xef:
        plan.handled = true;
        if (context.service_state != 4) {
            push(plan, MusicDispatchEffect::write_context_u32, kContext, 0x24, 0);
            call(plan, 0x03061ed2, 0);
            push(plan, MusicDispatchEffect::write_indirect_u16, 0x03089dbc, 0, 4);
            push(plan, MusicDispatchEffect::poll_event_with_limit, 0x030569bc, 0, 5, 20000);
            push(plan, MusicDispatchEffect::delay, 0x030661bc, 0, 1);
            push(plan, MusicDispatchEffect::clear_indirect_u32, 0x03089ddc);
            push(plan, MusicDispatchEffect::clear_indirect_u32, 0x03089de0);
        }
        break;
    case 0xf0:
        handler(0x03089492);
        break;
    case 0xf1:
        plan.handled = true;
        plan.effective_argument &= 0xffffU;
        call(plan, 0x0308966e, plan.effective_argument);
        push(plan, MusicDispatchEffect::call_after_nonzero_result,
             0x0308c84c, 0x0c, 0x03089dcc, 4);
        break;
    case 0xf9:
    case 0xfa:
        plan.handled = true;
        push(plan, MusicDispatchEffect::write_indirect_u32, kSharedState, 0, 2);
        call(plan, message_id == 0xf9 ? 0x030895ea : 0x03089586, argument);
        break;
    case 0xfb:
        plan.handled = true;
        if (plan.effective_argument > context.item_upper_bound) {
            plan.effective_argument = context.item_upper_bound;
        }
        call(plan, 0x0308ad5c, plan.effective_argument, 0);
        break;
    case 0xfc:
        plan.handled = true;
        push(plan, MusicDispatchEffect::call_with_message_slice, 0x0308ad7a, 4);
        push(plan, MusicDispatchEffect::write_indirect_u32, kSelection, 0, 1);
        push(plan, MusicDispatchEffect::write_indirect_u32, kSelection, 4,
             context.current_item_index);
        push(plan, MusicDispatchEffect::notify_event, 0x03068fdc, 0, 0x106);
        break;
    case 0xfd:
        plan.handled = true;
        if (context.selection_state == 1) {
            push(plan, MusicDispatchEffect::call_with_message_slice, 0x0308ad7a, 4);
            push(plan, MusicDispatchEffect::write_indirect_u32, kSelection, 0, 2);
            push(plan, MusicDispatchEffect::write_indirect_u32, kSelection, 8,
                 context.current_item_index);
        }
        push(plan, MusicDispatchEffect::copy_global_u32, kContext, 0x34, 0x03089dd0);
        break;
    case 0xfe:
        plan.handled = true;
        push(plan, MusicDispatchEffect::write_indirect_u32, kSelection, 0, 0);
        break;
    case 0xff:
        plan.handled = true;
        push(plan, MusicDispatchEffect::copy_global_byte_to_indirect,
             kSelection, 4, 0x03089dd4, 0x12);
        push(plan, MusicDispatchEffect::copy_global_byte,
             kModeBytePointer, 0, 0x03089dd4, 0x12);
        push(plan, MusicDispatchEffect::clear_event, 0x0306901e, 0, 0x2b);
        call(plan, 0x03088ebc);
        break;
    case 0x101:
        plan.handled = true;
        if (argument != 0) {
            push(plan, MusicDispatchEffect::copy_message_bytes,
                 kPayloadDestination, 40, 13);
        }
        call(plan, 0x03087ee4,
             static_cast<std::uint32_t>(static_cast<std::int32_t>(context.callback_argument)));
        push(plan, MusicDispatchEffect::clear_indirect_u8, kModeBytePointer);
        push(plan, MusicDispatchEffect::notify_event, 0x03068fdc, 0, 0x2b);
        break;
    case 0x10d:
        plan.handled = true;
        push(plan, MusicDispatchEffect::call_with_message_slice, 0x0308ad7a, 4);
        break;
    case 0x110:
        plan.handled = true;
        if (context.context_value_at_0x4c != 0) {
            const auto signed_byte = static_cast<std::int8_t>(context.context_value_at_0x4c);
            call(plan, 0x03059c74,
                 static_cast<std::uint32_t>(static_cast<std::int32_t>(signed_byte)));
        }
        push(plan, MusicDispatchEffect::write_indirect_u32, kSharedState, 0, 2);
        call(plan, 0x0308966e, 1);
        call(plan, 0x0306228c);
        call(plan, 0x03088b48, argument);
        call(plan, 0x0306228c);
        push(plan, MusicDispatchEffect::notify_event, 0x03068fdc, 0, 0x110);
        push(plan, MusicDispatchEffect::notify_event, 0x03068fdc, 0, 0x2b);
        break;
    case 0x111:
        handler(0x0306207c);
        break;
    default:
        break;
    }
    return plan;
}

} // namespace echo::audio
