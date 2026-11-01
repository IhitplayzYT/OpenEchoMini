#include "echo/runtime/module0_cleanup_list.hpp"

namespace echo::runtime {

bool module0_cleanup_list_contains(
    const Module0CleanupList& list,
    const Module0CleanupNode* candidate) noexcept {
    for (auto* node = list.head; node != nullptr; node = node->next) {
        if (node == candidate) {
            return true;
        }
    }
    return false;
}

void drain_module0_cleanup_list(Module0CleanupList& list) {
    auto* tail = list.head;
    while (tail != nullptr && tail->next != nullptr) {
        tail = tail->next;
    }

    while (tail != nullptr) {
        if (tail->cleanup) {
            tail->cleanup();
        }

        auto* previous = tail->previous;
        if (previous != nullptr) {
            previous->next = nullptr;
        }
        tail->previous = nullptr;
        tail->next = nullptr;
        tail = previous;
    }

    list.head = nullptr;
}

} // namespace echo::runtime
