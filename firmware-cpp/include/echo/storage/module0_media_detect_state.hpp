#pragma once

namespace echo::storage {

struct Module0MediaDetectState {
    bool last_sample{};
    bool change_pending{};
};

struct Module0MediaDetectResult {
    Module0MediaDetectState state{};
    bool present{};
    bool changed{};
};

// FUN_0305f246 samples the caller-supplied GPIO-derived state, latches changes,
// and selector 2 consumes the latch. This model does not read hardware.
[[nodiscard]] inline Module0MediaDetectResult module0_media_detect_update(
    Module0MediaDetectState state, bool sampled_present,
    bool consume_change) noexcept {
    if (state.last_sample != sampled_present) {
        state.change_pending = true;
        state.last_sample = sampled_present;
    }
    const bool changed = consume_change && state.change_pending;
    if (consume_change) state.change_pending = false;
    return {state, sampled_present, changed};
}

} // namespace echo::storage
