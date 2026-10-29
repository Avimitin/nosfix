#pragma once

#include <cstdint>

namespace nosfix {

struct RenderTailCalls {
    void* context;
    char (*has_entry)(void* context, void* list);
    char (*transition)(void* context, void* list, int state);
    void* (*dispatch)(void* context, void* process);
};

// Replays the stateful tail skipped from each repeated render. The process and
// list cursor fields intentionally use the verified game layout for this build.
void DispatchRenderTail(void* app, const RenderTailCalls& calls) noexcept;

} // namespace nosfix
