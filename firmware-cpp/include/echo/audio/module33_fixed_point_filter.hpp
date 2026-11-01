#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace echo::audio {

// Module 33 keeps each filter accumulator as a signed 64-bit two's-complement
// value. The explicit limbs let the model reproduce the target's modulo-2^64
// multiply and arithmetic Q13 shift without relying on overflowing C++ signed
// arithmetic.
struct Module33FixedState {
    std::int32_t high{};
    std::uint32_t low{};
};

struct Module33ChannelState {
    std::int16_t previous_input{};
    std::int16_t input_before_previous{};
    Module33FixedState first_history{};
    Module33FixedState second_history{};
};

struct Module33StereoState {
    std::array<Module33ChannelState, 2> channels{};
};

// Processes interleaved signed 16-bit PCM in place. The firmware calls the
// channel-0 loop at the base pointer, then calls the same loop at base + 1
// halfword. Each channel retains its input and accumulator history across
// calls. Returns the number of complete stereo frames processed.
[[nodiscard]] std::size_t process_module33_fixed_point_filter(
    std::span<std::int16_t> interleaved_pcm,
    std::size_t frame_count,
    Module33StereoState& state) noexcept;

} // namespace echo::audio
