#include "echo/audio/module33_codec_dispatch.hpp"

namespace echo::audio {

Module33CodecDispatchPlan plan_module33_codec_dispatch(
    std::uint8_t selector,
    Module33CodecOperation operation,
    std::uint32_t argument0,
    std::uint32_t argument1) noexcept {
    return {
        selector,
        operation,
        {argument0, argument1},
        selector != 0xff,
    };
}

} // namespace echo::audio
