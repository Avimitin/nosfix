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
    const auto expected_ticks = seconds * nosfix::FrameScheduler::kLegacySimulationHz;
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
            if (update_cost) now += kQpcFrequency / 1000; // 1 ms of legacy update work.
            break;
        }
    }
    return renders;
}

void TestRate(unsigned hz) {
    const auto one_second = SimulateSeconds(hz, 1, false, true);
    Check(one_second == hz);

    const auto ten_minutes = SimulateSeconds(hz, 600, false, true);
    Check(ten_minutes == static_cast<std::uint64_t>(hz) * 600);

    const auto jittered = SimulateSeconds(hz, 4, true, true);
    Check(jittered <= static_cast<std::uint64_t>(hz) * 4);
    Check(jittered >= static_cast<std::uint64_t>(hz) * 4 * 9 / 10);
}

void TestStallAdvancesOneTickAtATime() {
    nosfix::FrameScheduler scheduler;
    Check(scheduler.Start(kQpcFrequency, 360, 0));

    const auto stalled_now = kQpcFrequency / 2;
    const auto first = scheduler.Next(stalled_now);
    Check(first.action == nosfix::ScheduleAction::TickComplete);
    Check(first.late_by_qpc >= kQpcFrequency / 4);
    auto previous_boundary = scheduler.NextSimulationBoundary();

    // A 500 ms pause is retired as one boundary per original game iteration.
    for (int i = 0; i < 5; ++i) {
        const auto step = scheduler.Next(stalled_now);
        Check(step.action == nosfix::ScheduleAction::TickComplete);
        Check(step.late_by_qpc == stalled_now - previous_boundary);
        const auto next_boundary = scheduler.NextSimulationBoundary();
        const auto increment = next_boundary - previous_boundary;
        Check(increment >= kQpcFrequency / 60);
        Check(increment <= (kQpcFrequency + 59) / 60);
        previous_boundary = next_boundary;
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

    constexpr unsigned tick_count = 60;
    for (unsigned i = 0; i < tick_count; ++i) {
        const auto last_render_result =
            nosfix::RunScheduledIteration(scheduler, callbacks);
        Check(last_render_result > 0);
        state.now += kQpcFrequency / 1000; // 1 ms of update work.
    }
    Check(state.dispatches == tick_count);
    Check(state.renders == hz);
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

void TestFractionalSimulationPhase() {
    nosfix::FrameScheduler scheduler;
    Check(scheduler.SimulationPhase(0) == 0.0);
    // Non-divisible QPC frequency exercises rational boundary rounding.
    constexpr std::int64_t frequency = 10'000'003;
    constexpr std::int64_t epoch = 9'876'543'210;
    Check(scheduler.Start(frequency, 165, epoch));
    for (std::int64_t tick = 0; tick < 60 * 600; ++tick) {
        const auto start = epoch + tick * frequency / 60;
        const auto end = epoch + (tick + 1) * frequency / 60;
        Check(scheduler.SimulationPhase(start) == 0.0);
        const auto middle = start + (end - start) / 2;
        const double fraction = scheduler.SimulationPhase(middle);
        Check(fraction > 0.4999 && fraction <= 0.5);
        Check(scheduler.SimulationPhase(end + frequency) == 1.0);
        Check(scheduler.Next(end).action == nosfix::ScheduleAction::TickComplete);
    }
}

} // namespace

int main() {
    for (const auto hz : kRates) TestRate(hz);
    TestStallAdvancesOneTickAtATime();
    for (const auto hz : kRates) TestDispatchExactlyOnce(hz);
    TestRenderTailDispatch();
    TestFractionalSimulationPhase();
}
