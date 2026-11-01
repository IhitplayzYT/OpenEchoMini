#pragma once

#include <cstdint>
#include <optional>

namespace echo::image {

// Module 20's first Thumb trampoline converts an element-count request into a
// byte-count request before forwarding it to the callback installed by module
// 18. The caller can invoke the returned argument tuple through its own file
// or memory source adapter.
struct Module20ReadCallbackCall {
    std::uint32_t destination{};
    std::uint32_t byte_count{};
    std::uint32_t stream_id{};
};

[[nodiscard]] std::optional<Module20ReadCallbackCall>
make_module20_read_callback_call(std::uint32_t destination,
                                 std::uint32_t element_size,
                                 std::uint32_t element_count,
                                 std::uint32_t stream_id) noexcept;

// Module 0's seek callback accepts (offset, origin, stream_id); module 20's
// trampoline accepts these in stream_id/offset/origin order and rearranges
// them for the installed callback.
struct Module20SeekCallbackCall {
    std::uint32_t offset{};
    std::uint32_t origin{};
    std::uint32_t stream_id{};
};

[[nodiscard]] constexpr Module20SeekCallbackCall
make_module20_seek_callback_call(std::uint32_t stream_id,
                                std::uint32_t offset,
                                std::uint32_t origin) noexcept {
    return {offset, origin, stream_id};
}

} // namespace echo::image
