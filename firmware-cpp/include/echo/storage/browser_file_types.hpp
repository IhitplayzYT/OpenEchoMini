#pragma once

#include <array>
#include <string_view>

namespace echo::storage {

// Extensions packed into module 31's file-browser filter table.
[[nodiscard]] const std::array<std::string_view, 15>& browser_file_extensions() noexcept;
[[nodiscard]] const std::array<std::string_view, 16>& recording_file_extensions() noexcept;

} // namespace echo::storage
