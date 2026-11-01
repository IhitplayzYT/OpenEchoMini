#pragma once

#include <functional>

namespace echo::runtime {

struct Module0CleanupNode {
    Module0CleanupNode* previous = nullptr;
    Module0CleanupNode* next = nullptr;
    std::function<void()> cleanup;
};

struct Module0CleanupList {
    Module0CleanupNode* head = nullptr;
};

[[nodiscard]] bool module0_cleanup_list_contains(
    const Module0CleanupList& list,
    const Module0CleanupNode* candidate) noexcept;

// Mirrors the observed callback order: tail to head, then an empty list.
void drain_module0_cleanup_list(Module0CleanupList& list);

} // namespace echo::runtime
