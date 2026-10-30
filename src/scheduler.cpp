#include "nosfix/scheduler.hpp"

#include <algorithm>
#include <limits>

namespace nosfix {
namespace {

bool IsSupportedRate(unsigned hz) noexcept {
    return hz >= 60 && hz <= 1000;
}

} // namespace

bool FrameScheduler::Start(std::int64_t qpc_frequency,
                           unsigned presentation_hz,
                           std::int64_t qpc_now) noexcept {
    if (qpc_frequency <= 0 || !IsSupportedRate(presentation_hz)) return false;
    qpc_frequency_ = qpc_frequency;
    epoch_qpc_ = qpc_now;
    simulation_index_ = 1;
    presentation_index_ = 0;
    presentation_hz_ = presentation_hz;
    running_ = true;
    return true;
}

std::int64_t FrameScheduler::Boundary(unsigned hz,
                                      std::int64_t index) const noexcept {
    // Split the product to avoid index * frequency overflowing for long runs.
    const std::int64_t whole = qpc_frequency_ / hz;
    const std::int64_t remainder = qpc_frequency_ % hz;
    const std::int64_t elapsed = index * whole + (index * remainder) / hz;
    if (elapsed > 0 && epoch_qpc_ > std::numeric_limits<std::int64_t>::max() - elapsed)
        return std::numeric_limits<std::int64_t>::max();
    return epoch_qpc_ + elapsed;
}

std::int64_t FrameScheduler::FirstSlotAfter(std::int64_t qpc_now) const noexcept {
    if (qpc_now < epoch_qpc_) return 0;
    const std::int64_t delta = qpc_now - epoch_qpc_;
    const std::int64_t whole_seconds = delta / qpc_frequency_;
    const std::int64_t remainder = delta % qpc_frequency_;
    const std::int64_t passed = whole_seconds * presentation_hz_ +
        (remainder * presentation_hz_) / qpc_frequency_;
    std::int64_t first = passed + 1;
    while (Boundary(presentation_hz_, first) <= qpc_now) ++first;
    while (first > 0 && Boundary(presentation_hz_, first - 1) > qpc_now) --first;
    return first;
}

std::int64_t FrameScheduler::NextSimulationBoundary() const noexcept {
    if (!running_) return 0;
    return Boundary(presentation_hz_, simulation_index_);
}

double FrameScheduler::SimulationPhase(std::int64_t qpc_now) const noexcept {
    if (!running_) return 0.0;
    const auto begin = Boundary(presentation_hz_, simulation_index_ - 1);
    const auto end = Boundary(presentation_hz_, simulation_index_);
    if (end <= begin || qpc_now <= begin) return 0.0;
    if (qpc_now >= end) return 1.0;
    return static_cast<double>(qpc_now - begin) / static_cast<double>(end - begin);
}

ScheduleStep FrameScheduler::Next(std::int64_t qpc_now) noexcept {
    if (!running_) return {ScheduleAction::TickComplete, qpc_now, 0};

    const std::int64_t simulation_boundary =
        Boundary(presentation_hz_, simulation_index_);
    const std::int64_t presentation_boundary =
        Boundary(presentation_hz_, presentation_index_);

    if (qpc_now >= simulation_boundary) {
        // Serve one display slot before retiring the native update. A render
        // that overruns later boundaries does not render again for this update.
        if (presentation_index_ <= simulation_index_ &&
            presentation_boundary <= qpc_now) {
            presentation_index_ = FirstSlotAfter(qpc_now);
            return {ScheduleAction::Render, qpc_now, 0};
        }
        const std::int64_t late_by = qpc_now - simulation_boundary;
        ++simulation_index_;
        return {ScheduleAction::TickComplete, simulation_boundary, late_by};
    }

    if (presentation_boundary < simulation_boundary &&
        presentation_boundary <= qpc_now) {
        presentation_index_ = FirstSlotAfter(qpc_now);
        return {ScheduleAction::Render, qpc_now, 0};
    }

    if (presentation_boundary < simulation_boundary) {
        return {ScheduleAction::Wait, presentation_boundary, 0};
    }
    return {ScheduleAction::Wait, simulation_boundary, 0};
}

std::intptr_t RunScheduledIteration(FrameScheduler& scheduler,
                                    const IterationCallbacks& callbacks) {
    if (!callbacks.now || !callbacks.wait_until || !callbacks.render ||
        !callbacks.dispatch_once) return 0;

    std::intptr_t result = 0;
    for (;;) {
        const ScheduleStep step = scheduler.Next(callbacks.now(callbacks.context));
        switch (step.action) {
        case ScheduleAction::Wait:
            callbacks.wait_until(callbacks.context, step.deadline_qpc);
            break;
        case ScheduleAction::Render:
            result = callbacks.render(callbacks.context);
            break;
        case ScheduleAction::TickComplete:
            callbacks.dispatch_once(callbacks.context, step.late_by_qpc);
            return result;
        }
    }
}

} // namespace nosfix
