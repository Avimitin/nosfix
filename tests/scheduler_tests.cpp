#include "nosfix/scheduler.hpp"
#include "nosfix/render_tail.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace {

constexpr std::int64_t kQpcFrequency = 10'000'003;
constexpr std::array<unsigned, 7> kRates{60, 90, 120, 144, 165, 240, 360};

void CheckImpl(bool condition, const char* expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "scheduler test failed at line %d: %s\n", line, expression);
        std::abort();
    }
}
#define Check(condition) CheckImpl((condition), #condition, __LINE__)

std::uint64_t SimulateSeconds(unsigned hz, unsigned seconds, bool jitter,
                              bool update_cost) {
    nosfix::FrameScheduler scheduler;
    Check(scheduler.Start(kQpcFrequency, hz, 0));
    std::int64_t now = 0;
    std::uint64_t renders = 0;
    const auto expected_ticks = static_cast<std::uint64_t>(seconds) * hz;
    unsigned ticks = 0;
    std::int64_t wait_number = 0;
    while (ticks < expected_ticks) {
        const auto step = scheduler.Next(now);
        switch (step.action) {
        case nosfix::ScheduleAction::Render:
            ++renders;
            break;
        case nosfix::ScheduleAction::Wait:
            ++wait_number;
            now = step.deadline_qpc +
                (jitter ? (wait_number % 3) * (kQpcFrequency / 4000) : 0);
            break;
        case nosfix::ScheduleAction::TickComplete:
            ++ticks;
            now = std::max(now, step.deadline_qpc);
            if (update_cost) now += kQpcFrequency / 1000;
            break;
        }
    }
    return renders;
}

void TestRate(unsigned hz) {
    const auto one_second = SimulateSeconds(hz, 1, false, true);
    Check(one_second == static_cast<std::uint64_t>(hz) + 1);

    const auto ten_minutes = SimulateSeconds(hz, 600, false, true);
    Check(ten_minutes == static_cast<std::uint64_t>(hz) * 600 + 1);

    const auto jittered = SimulateSeconds(hz, 4, true, true);
    Check(jittered <= static_cast<std::uint64_t>(hz) * 4 + 1);
    Check(jittered >= static_cast<std::uint64_t>(hz) * 4 * 9 / 10);
}

void TestStallAdvancesOneTickAtATime() {
    nosfix::FrameScheduler scheduler;
    Check(scheduler.Start(kQpcFrequency, 360, 0));

    const auto stalled_now = kQpcFrequency / 2;
    auto boundary = scheduler.NextSimulationBoundary();

    // A 500 ms pause is retired as one native boundary per iteration. The
    // equal-rate scheduler may render before each tick; that is not a fallback.
    for (int i = 0; i < 5; ++i) {
        nosfix::ScheduleStep step{};
        do {
            step = scheduler.Next(stalled_now);
        } while (step.action == nosfix::ScheduleAction::Render);
        Check(step.action == nosfix::ScheduleAction::TickComplete);
        Check(step.deadline_qpc == boundary);
        Check(step.late_by_qpc == stalled_now - boundary);
        const auto next_boundary = scheduler.NextSimulationBoundary();
        const auto increment = next_boundary - boundary;
        Check(increment >= kQpcFrequency / 360);
        Check(increment <= (kQpcFrequency + 359) / 360);
        boundary = next_boundary;
    }
}

struct CallbackState {
    std::int64_t now = 0;
    unsigned renders = 0;
    unsigned dispatches = 0;
};

std::int64_t CallbackNow(void* context) {
    return static_cast<CallbackState*>(context)->now;
}

void CallbackWait(void* context, std::int64_t deadline) {
    auto& state = *static_cast<CallbackState*>(context);
    state.now = std::max(state.now, deadline);
}

std::intptr_t CallbackRender(void* context) {
    return ++static_cast<CallbackState*>(context)->renders;
}

void CallbackDispatch(void* context, std::int64_t) {
    ++static_cast<CallbackState*>(context)->dispatches;
}

void TestDispatchExactlyOnce(unsigned hz) {
    nosfix::FrameScheduler scheduler;
    Check(scheduler.Start(kQpcFrequency, hz, 0));
    CallbackState state;
    const nosfix::IterationCallbacks callbacks{
        &state, CallbackNow, CallbackWait, CallbackRender, CallbackDispatch};

    const unsigned tick_count = hz;
    for (unsigned i = 0; i < tick_count; ++i) {
        const auto last_render_result =
            nosfix::RunScheduledIteration(scheduler, callbacks);
        Check(last_render_result > 0);
        state.now += kQpcFrequency / 100000; // 10 us of update work.
    }
    Check(state.dispatches == tick_count);
    Check(state.renders >= tick_count);
    Check(state.renders <= tick_count + 1);
}

struct FakeListState {
    unsigned entry_count = 0;
    unsigned transition_count = 0;
    unsigned dispatch_count = 0;
};

char FakeHasEntry(void* context, void* list) {
    auto& state = *static_cast<FakeListState*>(context);
    const auto index = *reinterpret_cast<std::uint32_t*>(
        static_cast<unsigned char*>(list) + 80);
    return index < state.entry_count;
}

char FakeTransition(void* context, void*, int to_state) {
    Check(to_state == 2);
    ++static_cast<FakeListState*>(context)->transition_count;
    return 1;
}

void* FakeProcessDispatch(void* context, void*) {
    ++static_cast<FakeListState*>(context)->dispatch_count;
    return nullptr;
}

template <typename T>
void Store(std::byte* destination, std::size_t offset, T value) {
    std::memcpy(destination + offset, &value, sizeof(value));
}

void TestRenderTailDispatch() {
    alignas(8) std::array<std::byte, 192> app{};
    alignas(8) std::array<std::byte, 24> process{};
    alignas(8) std::array<std::byte, 88> list{};
    alignas(8) std::array<std::byte, 8> vector{};
    alignas(8) std::array<std::byte, 3 * 24> entries{};
    void* process_ptr = process.data();
    void* list_ptr = list.data();
    void* vector_ptr = vector.data();
    void* entries_ptr = entries.data();
    Store(app.data(), 176, process_ptr);
    Store(process.data(), 16, list_ptr);
    Store(list.data(), 8, vector_ptr);
    Store(vector.data(), 0, entries_ptr);
    const std::array<std::uint32_t, 3> states{0, 1, 4};
    for (std::size_t i = 0; i < states.size(); ++i)
        Store(entries.data(), 24 * i + 4, states[i]);

    *reinterpret_cast<std::uint32_t*>(list.data() + 80) = 77;
    *reinterpret_cast<unsigned char*>(list.data() + 84) = 6;
    FakeListState state{3, 0, 0};
    const nosfix::RenderTailCalls calls{
        &state, FakeHasEntry, FakeTransition, FakeProcessDispatch};
    nosfix::DispatchRenderTail(app.data(), calls);

    Check(state.transition_count == 2);
    Check(state.dispatch_count == 1);
    Check(*reinterpret_cast<std::uint32_t*>(list.data() + 80) == 3);
    Check(*reinterpret_cast<unsigned char*>(list.data() + 84) == 0);
}

void TestEqualRateRenderThenDispatch() {
    constexpr unsigned hz = 120;
    nosfix::FrameScheduler scheduler;
    Check(scheduler.Start(kQpcFrequency, hz, 0));
    Check(scheduler.SimulationHz() == hz);
    std::int64_t now = 0;
    unsigned renders = 0;
    unsigned ticks = 0;
    while (ticks < hz) {
        const auto step = scheduler.Next(now);
        switch (step.action) {
        case nosfix::ScheduleAction::Render:
            ++renders;
            break;
        case nosfix::ScheduleAction::Wait:
            now = step.deadline_qpc;
            break;
        case nosfix::ScheduleAction::TickComplete:
            ++ticks;
            now = std::max(now, step.deadline_qpc);
            now += kQpcFrequency / 10000; // 100 us update work
            break;
        }
    }
    Check(renders >= ticks);
    Check(renders <= ticks + 1);
}

void TestFractionalSimulationPhase() {
    nosfix::FrameScheduler scheduler;
    Check(scheduler.SimulationPhase(0) == 0.0);
    // Non-divisible QPC frequency exercises rational boundary rounding.
    constexpr std::int64_t frequency = 10'000'003;
    constexpr std::int64_t epoch = 9'876'543'210;
    constexpr unsigned hz = 165;
    Check(scheduler.Start(frequency, hz, epoch));
    for (std::int64_t tick = 0; tick < hz * 600; ++tick) {
        const auto start = epoch + tick * frequency / hz;
        const auto end = epoch + (tick + 1) * frequency / hz;
        Check(scheduler.SimulationPhase(start) == 0.0);
        const auto middle = start + (end - start) / 2;
        const double fraction = scheduler.SimulationPhase(middle);
        Check(fraction > 0.4999 && fraction <= 0.5);
        Check(scheduler.SimulationPhase(end + frequency) == 1.0);
        for (;;) {
            const auto step = scheduler.Next(end);
            if (step.action == nosfix::ScheduleAction::TickComplete) break;
        }
    }
}

} // namespace

int main() {
    for (const auto hz : kRates) TestRate(hz);
    TestStallAdvancesOneTickAtATime();
    for (const auto hz : kRates) TestDispatchExactlyOnce(hz);
    TestRenderTailDispatch();
    TestEqualRateRenderThenDispatch();
    TestFractionalSimulationPhase();
}
