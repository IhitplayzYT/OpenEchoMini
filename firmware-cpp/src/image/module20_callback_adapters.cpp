#include "echo/image/module20_callback_adapters.hpp"

#include <limits>

namespace echo::image {

std::optional<Module20ReadCallbackCall> make_module20_read_callback_call(
    std::uint32_t destination,
    std::uint32_t element_size,
    std::uint32_t element_count,
    std::uint32_t stream_id) noexcept {
    if (element_size != 0 &&
        element_count > std::numeric_limits<std::uint32_t>::max() / element_size) {
        return std::nullopt;
    }
    return Module20ReadCallbackCall{
        destination,
        element_size * element_count,
        stream_id,
    };
}

} // namespace echo::image
