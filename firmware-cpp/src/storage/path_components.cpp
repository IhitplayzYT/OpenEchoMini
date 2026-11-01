#include "echo/storage/path_components.hpp"

namespace echo::storage {

bool split_firmware_path(
    std::span<const char16_t> path,
    FirmwarePathComponents& output) noexcept {
    output = {};
    std::size_t component = 0;
    std::size_t offset = 0;

    for (const char16_t ch : path) {
        if (ch == u'\0') {
            output.count = component + (offset != 0 ? 1U : 0U);
            return true;
        }
        if (ch == u'/' || ch == u'\\') {
            if (offset != 0) {
                auto& current = output.values[component];
                current[offset] = u'\0';
                const bool single_dot = offset == 1 && current[0] == u'.';
                if (!single_dot) {
                    ++component;
                    if (component >= FirmwarePathComponents::max_components) {
                        output = {};
                        return false;
                    }
                }
                offset = 0;
            }
            continue;
        }

        if (offset >= FirmwarePathComponents::component_capacity - 1) {
            continue; // Firmware stops appending after 63 UTF-16 code units.
        }
        output.values[component][offset++] = ch;
    }

    output = {};
    return false; // The observed caller expects a NUL-terminated path.
}

} // namespace echo::storage
