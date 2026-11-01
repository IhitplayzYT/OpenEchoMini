#include "echo/image/jpeg_stream_buffer.hpp"

namespace echo::image {

bool module19_pad_short_read_with_eoi(
    const std::span<std::uint8_t> buffer_with_slack,
    const std::size_t bytes_read) noexcept {
    if (bytes_read >= module19_input_chunk_bytes
        || bytes_read > buffer_with_slack.size()
        || module19_eoi_padding_bytes > buffer_with_slack.size() - bytes_read) {
        return false;
    }
    for (std::size_t i = 0; i < module19_eoi_padding_bytes; i += 2U) {
        buffer_with_slack[bytes_read + i] = 0xffU;
        buffer_with_slack[bytes_read + i + 1U] = 0xd9U;
    }
    return true;
}

} // namespace echo::image
