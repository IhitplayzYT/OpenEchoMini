#include "echo/storage/cue_text_encoding.hpp"

namespace echo::storage {

CueTextEncodingInfo detect_cue_text_encoding(
    const std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() >= 2 && bytes[0] == 0xff && bytes[1] == 0xfe) {
        return {CueTextEncoding::utf16_little_endian, 2};
    }
    if (bytes.size() >= 2 && bytes[0] == 0xfe && bytes[1] == 0xff) {
        return {CueTextEncoding::utf16_big_endian, 2};
    }
    if (bytes.size() >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb
        && bytes[2] == 0xbf) {
        return {CueTextEncoding::utf8, 3};
    }
    return {};
}

} // namespace echo::storage
