#include "echo/storage/module31_playlist.hpp"

namespace echo::storage {
namespace {

constexpr bool is_ascii_space(const char16_t c) noexcept {
    return c == u' ' || c == u'\t';
}

constexpr bool is_ascii_letter(const char16_t c) noexcept {
    return (c >= u'A' && c <= u'Z') || (c >= u'a' && c <= u'z');
}

constexpr bool is_separator(const char16_t c) noexcept {
    return c == u'/' || c == u'\\';
}

} // namespace

Module31Bom detect_module31_bom(
    const std::span<const std::uint8_t> prefix) noexcept {
    if (prefix.size() >= 3 && prefix[0] == 0xef && prefix[1] == 0xbb &&
        prefix[2] == 0xbf) {
        return {Module31BomKind::utf8, 3};
    }
    if (prefix.size() >= 2 && prefix[0] == 0xff && prefix[1] == 0xfe) {
        return {Module31BomKind::utf16_little_endian, 2};
    }
    if (prefix.size() >= 2 && prefix[0] == 0xfe && prefix[1] == 0xff) {
        return {Module31BomKind::utf16_big_endian, 2};
    }
    return {};
}

Module31PlaylistLinePlan plan_module31_playlist_line(
    const std::u16string_view decoded_line) {
    Module31PlaylistLinePlan plan{};
    std::size_t begin = 0;
    std::size_t end = decoded_line.size();
    while (begin < end && is_ascii_space(decoded_line[begin])) ++begin;
    while (end > begin && is_ascii_space(decoded_line[end - 1])) --end;
    if (begin == end || decoded_line[begin] == u'#') {
        plan.ignored = true;
        return plan;
    }

    const auto length = end - begin;
    plan.trimmed_line.assign(decoded_line.substr(
        begin, length < module31_max_playlist_path_units
                   ? length
                   : module31_max_playlist_path_units));
    auto path = plan.trimmed_line;
    bool explicit_path = false;
    if (path.size() >= 2 && is_ascii_letter(path[0]) && path[1] == u':') {
        path.erase(0, 2);
        explicit_path = true;
    } else if (!path.empty() && is_separator(path.front())) {
        path.erase(0, 1);
        explicit_path = true;
    }

    const auto first_root = explicit_path
        ? Module31PlaylistRoot::opaque_path_helper
        : Module31PlaylistRoot::playlist_context;
    plan.attempts[plan.attempt_count++] = {path, first_root, explicit_path};

    if (explicit_path) {
        plan.attempts[plan.attempt_count++] = {
            path, Module31PlaylistRoot::playlist_context, false};
        std::size_t last_separator = std::u16string::npos;
        for (std::size_t i = 0; i < path.size(); ++i) {
            if (is_separator(path[i])) last_separator = i;
        }
        if (last_separator != std::u16string::npos) {
            plan.attempts[plan.attempt_count++] = {
                path.substr(last_separator + 1),
                Module31PlaylistRoot::playlist_context, false};
        }
    }
    return plan;
}

} // namespace echo::storage
