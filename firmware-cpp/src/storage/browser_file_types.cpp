#include "echo/storage/browser_file_types.hpp"

namespace echo::storage {

const std::array<std::string_view, 15>& browser_file_extensions() noexcept {
    static constexpr std::array<std::string_view, 15> extensions{
        "MP1", "MP2", "MP3", "WMA", "WAV", "APE", "FLA", "AAC",
        "M4A", "OGG", "MP4", "3GP", "DFF", "DSF", "M3U"};
    return extensions;
}

const std::array<std::string_view, 16>& recording_file_extensions() noexcept {
    static constexpr std::array<std::string_view, 16> extensions{
        "MP1", "MP2", "MP3", "WMA", "WAV", "APE", "FLA", "AAC",
        "M4A", "OGG", "MP4", "3GP", "DFF", "DSF", "M3U", "CUE"};
    return extensions;
}

} // namespace echo::storage
