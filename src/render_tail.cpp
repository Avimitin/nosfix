#include "nosfix/render_tail.hpp"

#include <cstddef>

namespace nosfix {
namespace {

constexpr std::size_t kAppProcessOffset = 176;
constexpr std::size_t kProcessListOffset = 16;
constexpr std::size_t kListEntriesOffset = 8;
constexpr std::size_t kListIndexOffset = 80;
constexpr std::size_t kListRowOffset = 84;
constexpr std::size_t kEntryStride = 24;
constexpr std::size_t kEntryStateOffset = 4;
} // namespace

void DispatchRenderTail(void* app, const RenderTailCalls& calls) noexcept {
    if (!app || !calls.has_entry || !calls.transition || !calls.dispatch) return;
    auto* app_bytes = static_cast<unsigned char*>(app);
    void* process = *reinterpret_cast<void**>(app_bytes + kAppProcessOffset);
    if (!process) return;

    auto* process_bytes = static_cast<unsigned char*>(process);
    void* list = *reinterpret_cast<void**>(process_bytes + kProcessListOffset);
    if (!list) return;
    auto* list_bytes = static_cast<unsigned char*>(list);
    *reinterpret_cast<unsigned char*>(list_bytes + kListRowOffset) = 0;
    *reinterpret_cast<std::uint32_t*>(list_bytes + kListIndexOffset) = 0;

    if (calls.has_entry(calls.context, list)) {
        do {
            // The original loop reloads process->list and calls has_entry
            // before reading its row/index because that call may mutate them.
            process_bytes = static_cast<unsigned char*>(process);
            list = *reinterpret_cast<void**>(process_bytes + kProcessListOffset);
            if (!list) break;
            list_bytes = static_cast<unsigned char*>(list);
            std::uint32_t state = 0;
            if (calls.has_entry(calls.context, list)) {
                const auto row = *reinterpret_cast<unsigned char*>(list_bytes + kListRowOffset);
                const auto index = *reinterpret_cast<std::uint32_t*>(list_bytes + kListIndexOffset);
                auto* vector = *reinterpret_cast<unsigned char**>(
                    list_bytes + kListEntriesOffset + 8 * row);
                auto* entries = vector ? *reinterpret_cast<unsigned char**>(vector) : nullptr;
                if (entries) {
                    auto* entry = entries + kEntryStride * index;
                    state = *reinterpret_cast<std::uint32_t*>(entry + kEntryStateOffset);
                }
            }

            if (state == 1 || state == 4) calls.transition(calls.context, list, 2);

            process_bytes = static_cast<unsigned char*>(process);
            list = *reinterpret_cast<void**>(process_bytes + kProcessListOffset);
            if (!list || !calls.has_entry(calls.context, list)) break;
            list_bytes = static_cast<unsigned char*>(list);
            ++*reinterpret_cast<std::uint32_t*>(list_bytes + kListIndexOffset);
        } while (calls.has_entry(calls.context, list));
    }

    calls.dispatch(calls.context, process);
}

} // namespace nosfix
