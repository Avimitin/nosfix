#pragma once

#include <cstdint>

namespace nosfix {

enum class ScheduleAction {
    Render,
    Wait,
    TickComplete,
};

struct ScheduleStep {
    ScheduleAction action;
    std::int64_t deadline_qpc;
    std::int64_t late_by_qpc;
};

struct IterationCallbacks {
    void* context;
    std::int64_t (*now)(void* context);
    void (*wait_until)(void* context, std::int64_t deadline_qpc);
    std::intptr_t (*render)(void* context);
    void (*dispatch_once)(void* context, std::int64_t late_by_qpc);
};

// Drives one native simulation boundary and one presentation boundary at the
// display rate. There is no fixed 60 Hz simulation mode and no rate fallback.
// Rendering lateness is dropped; it never shifts the simulation timeline.
class FrameScheduler final {
public:
    [[nodiscard]] bool Start(std::int64_t qpc_frequency,
                             unsigned presentation_hz,
                             std::int64_t qpc_now) noexcept;
    [[nodiscard]] ScheduleStep Next(std::int64_t qpc_now) noexcept;

    [[nodiscard]] std::int64_t NextSimulationBoundary() const noexcept;
    [[nodiscard]] double SimulationPhase(std::int64_t qpc_now) const noexcept;
    [[nodiscard]] unsigned PresentationHz() const noexcept { return presentation_hz_; }
    [[nodiscard]] unsigned SimulationHz() const noexcept { return presentation_hz_; }
    [[nodiscard]] bool Running() const noexcept { return running_; }

private:
    [[nodiscard]] std::int64_t Boundary(unsigned hz,
                                        std::int64_t index) const noexcept;
    [[nodiscard]] std::int64_t FirstSlotAfter(std::int64_t qpc_now) const noexcept;

    std::int64_t epoch_qpc_ = 0;
    std::int64_t qpc_frequency_ = 0;
    std::int64_t simulation_index_ = 1;
    std::int64_t presentation_index_ = 0;
    unsigned presentation_hz_ = 0;
    bool running_ = false;
};

// Runs one scheduler interval. Each due presentation slot invokes render; the
// native tail is always dispatched exactly once before returning.
[[nodiscard]] std::intptr_t RunScheduledIteration(
    FrameScheduler& scheduler, const IterationCallbacks& callbacks);

} // namespace nosfix
