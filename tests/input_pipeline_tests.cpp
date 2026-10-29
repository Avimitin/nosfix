// Always-on Release tests for nosfix::InputPipeline.
//
// Synthetic devices only (synthetic_hid.hpp): no physical input is touched.
// Checks are counted, never <cassert>.

#include "synthetic_hid.hpp"

#include "nosfix/input_pipeline.hpp"
#include "nosfix/song_clock_publication.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <span>
#include <thread>
#include <vector>

namespace allocation_probe {
inline bool tracking = false;
inline std::uint64_t count = 0;
} // namespace allocation_probe

void* operator new(std::size_t size) {
    if (allocation_probe::tracking) ++allocation_probe::count;
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) throw std::bad_alloc();
    return memory;
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace {

int g_checks = 0;
int g_failures = 0;

#define NOSFIX_CHECK(condition)                                                   \
    do {                                                                          \
        ++g_checks;                                                               \
        if (!(condition)) {                                                       \
            ++g_failures;                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        }                                                                         \
    } while (false)

using namespace nosfix;
using synthetic::ButtonCapSpec;
using synthetic::DeviceSpec;
using synthetic::Registry;
using synthetic::SyntheticProbe;
using synthetic::ValueCapSpec;

constexpr std::int64_t kFrequency = 10'000'000;
constexpr std::int64_t kAnchor = 1'000'000'000'000LL;
constexpr std::uint64_t kKeyboardHandle = 0x100;
constexpr std::uint64_t kKeyboardSource = 0xAAAA'0001;
constexpr std::uint64_t kHidHandle = 0x200;
constexpr std::uint64_t kHidSource = 0xBBBB'0002;

std::uint64_t g_clock_version = 0;

RawInputCaptureClock Capture(const SongClock& clock) {
    RawInputCaptureClock capture;
    capture.segment = clock.Snapshot();
    capture.version = ++g_clock_version;
    capture.continuity = SongClockContinuity::Monotonic;
    capture.valid = true;
    return capture;
}

RawInputCaptureClock CaptureRegressed(const SongClock& clock) {
    RawInputCaptureClock capture = Capture(clock);
    capture.continuity = SongClockContinuity::PossiblyRegressed;
    return capture;
}

struct Fixture final {
    Registry registry;
    SyntheticProbe probe{registry};
    RawInputPacketQueue queue;
    HidApi api = Registry::Api();
    InputPipeline pipeline{queue, probe, api};

    std::uint64_t AddKeyboard(std::uint64_t handle, std::uint64_t source) {
        DeviceSpec spec;
        spec.keyboard = true;
        spec.source_id = source;
        const std::uint64_t token = registry.Add(std::move(spec));
        probe.Set(handle, token);
        return token;
    }

    std::uint64_t AddHid(std::uint64_t handle, DeviceSpec spec) {
        const std::uint64_t token = registry.Add(std::move(spec));
        probe.Set(handle, token);
        return token;
    }
};

struct Run final {
    std::vector<PipelineInputEvent> events;
    bool ok = true;
    PipelineStep stop = PipelineStep::NoRecord;
};

Run ProcessAll(Fixture& fixture, std::size_t max_calls = 1'000'000) {
    Run run;
    std::array<PipelineInputEvent, 64> buffer{};
    for (std::size_t call = 0; call < max_calls; ++call) {
        const PipelineProcessResult result = fixture.pipeline.ProcessOne(buffer);
        run.events.insert(run.events.end(), buffer.begin(),
                          buffer.begin() + static_cast<std::ptrdiff_t>(result.events_written));
        run.stop = result.step;
        if (result.step == PipelineStep::Processed ||
            result.step == PipelineStep::NeedOutputCapacity) {
            continue;
        }
        if (result.step == PipelineStep::NoRecord) return run;
        run.ok = false;
        return run;
    }
    run.ok = false;
    return run;
}

void Append(Run& total, Run part) {
    total.ok = total.ok && part.ok;
    total.stop = part.stop;
    total.events.insert(total.events.end(), part.events.begin(), part.events.end());
}

void PushKey(Fixture& fixture, std::int64_t qpc, USHORT vkey, USHORT scan,
             USHORT flags, const RawInputCaptureClock& clock) {
    const auto payload = synthetic::KeyboardPayload(kKeyboardHandle, vkey, scan, flags);
    NOSFIX_CHECK(fixture.queue.PushRawInput(qpc, payload, clock));
}

void PushBreak(Fixture& fixture, std::int64_t qpc, USHORT vkey, USHORT scan,
               const RawInputCaptureClock& clock) {
    PushKey(fixture, qpc, vkey, scan, RI_KEY_BREAK, clock);
}

PipelineBindingSpec KeyBinding(std::uint64_t source, USHORT vkey, std::uint8_t lane) {
    PipelineBindingSpec spec;
    spec.source_id = source;
    spec.control_id = MakeKeyboardControlId(static_cast<std::uint32_t>(vkey));
    spec.lane = lane;
    spec.kind = InputControlKind::Button;
    return spec;
}

// ---------------------------------------------------------------------------

void TestRapidTapsAndImmutableMapping() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'A', 0);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto before = Capture(clock);

    PushKey(fixture, kAnchor + 10, 'A', 0x1E, 0, before);
    PushBreak(fixture, kAnchor + 20, 'A', 0x1E, before);
    PushKey(fixture, kAnchor + 30, 'A', 0x1E, 0, before); // a 0 us tap: both edges
    PushBreak(fixture, kAnchor + 30, 'A', 0x1E, before);

    // A late audio observation/seek must not retime the queued events.
    NOSFIX_CHECK(clock.Seek(kAnchor + 1'000, 5'000'000.0));
    const auto after = Capture(clock);
    PushKey(fixture, kAnchor + 1'010, 'A', 0x1E, 0, after);

    const Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.size() == 5);
    if (run.events.size() == 5) {
        const std::array<InputEdge, 5> edges{
            InputEdge::Press, InputEdge::Release, InputEdge::Press,
            InputEdge::Release, InputEdge::Press};
        for (std::size_t i = 0; i < 5; ++i) {
            NOSFIX_CHECK(run.events[i].input.lane == 0);
            NOSFIX_CHECK(run.events[i].input.edge == edges[i]);
            NOSFIX_CHECK(run.events[i].input.stamp.sequence == i + 1);
            NOSFIX_CHECK(run.events[i].input.stamp.ordinal == 0);
            NOSFIX_CHECK(run.events[i].input.source_id == kKeyboardSource);
            NOSFIX_CHECK(run.events[i].input.device_generation == 1);
            NOSFIX_CHECK(!run.events[i].input.synthetic);
            NOSFIX_CHECK(run.events[i].has_song_time);
        }
        NOSFIX_CHECK(std::abs(run.events[0].song_time.microseconds - 1.0) < 1e-6);
        NOSFIX_CHECK(std::abs(run.events[1].song_time.microseconds - 2.0) < 1e-6);
        NOSFIX_CHECK(std::abs(run.events[2].song_time.microseconds - 3.0) < 1e-6);
        NOSFIX_CHECK(std::abs(run.events[3].song_time.microseconds - 3.0) < 1e-6);
        NOSFIX_CHECK(run.events[2].song_time.generation == 1);
        NOSFIX_CHECK(run.events[4].song_time.generation == 2);
        NOSFIX_CHECK(std::abs(run.events[4].song_time.microseconds - 5'000'001.0) < 1e-3);
    }
    NOSFIX_CHECK(fixture.pipeline.Stats().faults == 0);
    NOSFIX_CHECK(fixture.pipeline.Stats().events_emitted == 5);
}

void TestTypematicAndReset() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'A', 3);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    for (int i = 0; i < 5; ++i) {
        PushKey(fixture, kAnchor + 10 + i, 'A', 0x1E, 0, capture); // typematic makes
    }
    PushBreak(fixture, kAnchor + 30, 'A', 0x1E, capture);
    const Run pressed = ProcessAll(fixture);
    NOSFIX_CHECK(pressed.ok);
    NOSFIX_CHECK(pressed.events.size() == 2);
    if (pressed.events.size() >= 2) {
        NOSFIX_CHECK(pressed.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(pressed.events[1].input.edge == InputEdge::Release);
    }

    // A stream-loss boundary releases a held lane synthetically.
    PushKey(fixture, kAnchor + 40, 'A', 0x1E, 0, capture);
    fixture.queue.MarkGap(kAnchor + 50, RawInputResetReason::DeviceChange, capture);
    const Run lost = ProcessAll(fixture);
    NOSFIX_CHECK(lost.ok);
    NOSFIX_CHECK(lost.events.size() == 2);
    if (lost.events.size() >= 2) {
        NOSFIX_CHECK(lost.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(lost.events[1].input.edge == InputEdge::Release);
        NOSFIX_CHECK(lost.events[1].input.synthetic);
        NOSFIX_CHECK(lost.events[1].input.lane == 3);
        NOSFIX_CHECK(!lost.events[1].input.velocity);
    }
    NOSFIX_CHECK(fixture.pipeline.Stats().synthetic_releases == 1);
}

void TestChordsAndAlternatives() {
    Fixture fixture;
    DeviceSpec spec;
    spec.source_id = kHidSource;
    spec.input_report_bytes = 3;
    spec.buttons = {ButtonCapSpec{1, 1, 0, true, 1, 4, 0}};
    const std::uint64_t token = fixture.AddHid(kHidHandle, spec);

    std::array<PipelineBindingSpec, 4> bindings{};
    bindings[0] = PipelineBindingSpec{kHidSource, MakeHidButtonControlId(0), 0};
    bindings[1] = PipelineBindingSpec{kHidSource, MakeHidButtonControlId(1), 1};
    bindings[2] = PipelineBindingSpec{kHidSource, MakeHidButtonControlId(2), 2};
    bindings[3] = PipelineBindingSpec{kHidSource, MakeHidButtonControlId(3), 0}; // alt lane 0
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings(bindings) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    Run run;
    // The synthetic HidP state is current-state based, so each report is
    // pushed and drained before the next report's usage set is installed.
    auto push_report = [&](const std::vector<USAGE>& usages, std::int64_t qpc) {
        fixture.registry.Get(token).SetOn(1, 1, 0, usages);
        const auto payload = synthetic::HidPayload(
            kHidHandle, 3, {synthetic::Report(1, 3)});
        NOSFIX_CHECK(fixture.queue.PushRawInput(qpc, payload, capture));
        Append(run, ProcessAll(fixture));
    };

    push_report({1, 2, 3}, kAnchor + 10);
    push_report({1, 2, 3, 4}, kAnchor + 20); // alt press: lane 0 stays held
    push_report({2, 3, 4}, kAnchor + 30);    // alt release: lane 0 stays held
    push_report({2}, kAnchor + 40);          // lane 2 and lane 0 release
    push_report({}, kAnchor + 50);           // lane 1 releases

    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.size() == 6);
    if (run.events.size() == 6) {
        for (std::size_t i = 0; i < 3; ++i) {
            NOSFIX_CHECK(run.events[i].input.lane == i);
            NOSFIX_CHECK(run.events[i].input.edge == InputEdge::Press);
            NOSFIX_CHECK(run.events[i].input.stamp.sequence == 1);
            NOSFIX_CHECK(run.events[i].input.stamp.ordinal == i);
            NOSFIX_CHECK(run.events[i].input.stamp.qpc == kAnchor + 10);
        }
        NOSFIX_CHECK(run.events[3].input.lane == 2);
        NOSFIX_CHECK(run.events[3].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[3].input.stamp.sequence == 4);
        NOSFIX_CHECK(run.events[4].input.lane == 0);
        NOSFIX_CHECK(run.events[4].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[4].input.stamp.sequence == 4);
        NOSFIX_CHECK(run.events[5].input.lane == 1);
        NOSFIX_CHECK(run.events[5].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[5].input.stamp.sequence == 5);
    }
    NOSFIX_CHECK(fixture.pipeline.Stats().events_emitted == 6);
}

void TestPausedPlayback() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'B', 5);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto running = Capture(clock);
    PushKey(fixture, kAnchor + 10, 'B', 0x30, 0, running);
    NOSFIX_CHECK(clock.Pause(kAnchor + 20));
    const auto paused = Capture(clock);
    PushBreak(fixture, kAnchor + 30, 'B', 0x30, paused);
    NOSFIX_CHECK(clock.Resume(kAnchor + 100));
    const auto resumed = Capture(clock);
    PushKey(fixture, kAnchor + 110, 'B', 0x30, 0, resumed);

    const Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.size() == 3);
    if (run.events.size() == 3) {
        NOSFIX_CHECK(run.events[0].has_song_time);
        NOSFIX_CHECK(!run.events[1].has_song_time); // capture during pause
        NOSFIX_CHECK(run.events[2].has_song_time);
    }
    const auto watermark = fixture.pipeline.Watermark();
    NOSFIX_CHECK(watermark.cursor.valid);
    NOSFIX_CHECK(watermark.cursor.sequence == 3);
    NOSFIX_CHECK(watermark.song.valid());
    NOSFIX_CHECK(watermark.song.sequence == 3);
    NOSFIX_CHECK(fixture.pipeline.Stats().clock_unmapped_records == 1);
}

void TestReportIdIsolation() {
    Fixture fixture;
    DeviceSpec spec;
    spec.source_id = kHidSource;
    spec.input_report_bytes = 3;
    spec.buttons = {
        ButtonCapSpec{1, 1, 0, false, 1, 1, 0},
        ButtonCapSpec{1, 2, 0, false, 1, 1, 0},
    };
    const std::uint64_t token = fixture.AddHid(kHidHandle, spec);
    std::array<PipelineBindingSpec, 2> bindings{
        PipelineBindingSpec{kHidSource, MakeHidButtonControlId(0), 0},
        PipelineBindingSpec{kHidSource, MakeHidButtonControlId(1), 1},
    };
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings(bindings) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    Run run;
    auto push = [&](UCHAR report_id, const std::vector<USAGE>& usages, std::int64_t qpc) {
        fixture.registry.Get(token).SetOn(report_id, 1, 0, usages);
        const auto payload = synthetic::HidPayload(
            kHidHandle, 3, {synthetic::Report(report_id, 3)});
        NOSFIX_CHECK(fixture.queue.PushRawInput(qpc, payload, capture));
        Append(run, ProcessAll(fixture));
    };
    push(1, {1}, kAnchor + 10);
    push(2, {}, kAnchor + 20);  // must not release report ID 1's lane 0
    push(1, {}, kAnchor + 30);  // now lane 0 releases

    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.size() == 2);
    if (run.events.size() == 2) {
        NOSFIX_CHECK(run.events[0].input.lane == 0);
        NOSFIX_CHECK(run.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(run.events[1].input.lane == 0);
        NOSFIX_CHECK(run.events[1].input.edge == InputEdge::Release);
    }
    NOSFIX_CHECK(fixture.pipeline.Stats().faults == 0);
}

void TestOutputCapacityRetry() {
    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);

    const std::uint64_t token = 0;
    (void)token;

    auto build = [&](Fixture& fixture) {
        DeviceSpec spec;
        spec.source_id = kHidSource;
        spec.input_report_bytes = 3;
        spec.buttons = {ButtonCapSpec{1, 1, 0, true, 1, 3, 0}};
        const std::uint64_t device_token = fixture.AddHid(kHidHandle, spec);
        std::array<PipelineBindingSpec, 3> bindings{
            PipelineBindingSpec{kHidSource, MakeHidButtonControlId(0), 0},
            PipelineBindingSpec{kHidSource, MakeHidButtonControlId(1), 1},
            PipelineBindingSpec{kHidSource, MakeHidButtonControlId(2), 2},
        };
        NOSFIX_CHECK(fixture.pipeline.ConfigureBindings(bindings) ==
                     PipelineConfigStatus::Accepted);
        fixture.registry.Get(device_token).SetOn(1, 1, 0, {1, 2, 3});
        const auto payload = synthetic::HidPayload(kHidHandle, 3,
                                                   {synthetic::Report(1, 3)});
        NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 10, payload, capture));
    };

    std::vector<PipelineInputEvent> reference;
    {
        Fixture fixture;
        build(fixture);
        reference = ProcessAll(fixture).events;
    }
    NOSFIX_CHECK(reference.size() == 3);

    Fixture fixture;
    build(fixture);
    std::vector<PipelineInputEvent> collected;
    std::array<PipelineInputEvent, 2> small{};
    const PipelineProcessResult first = fixture.pipeline.ProcessOne(small);
    NOSFIX_CHECK(first.events_written == 2);
    collected.insert(collected.end(), small.begin(),
                     small.begin() + static_cast<std::ptrdiff_t>(first.events_written));
    NOSFIX_CHECK(first.step == PipelineStep::NeedOutputCapacity);
    NOSFIX_CHECK(first.required_event_count == 1);
    NOSFIX_CHECK(fixture.pipeline.PendingTransitionCount() == 1);

    std::array<PipelineInputEvent, 1> one{};
    const PipelineProcessResult second = fixture.pipeline.ProcessOne(one);
    if (second.events_written == 1) collected.push_back(one[0]);
    NOSFIX_CHECK(second.step == PipelineStep::Processed);
    NOSFIX_CHECK(second.events_written == 1);
    NOSFIX_CHECK(collected.size() == reference.size());
    for (std::size_t i = 0; i < collected.size() && i < reference.size(); ++i) {
        NOSFIX_CHECK(collected[i].input.lane == reference[i].input.lane);
        NOSFIX_CHECK(collected[i].input.stamp.ordinal == reference[i].input.stamp.ordinal);
        NOSFIX_CHECK(collected[i].input.stamp.sequence == reference[i].input.stamp.sequence);
        NOSFIX_CHECK(collected[i].input.stamp.qpc == reference[i].input.stamp.qpc);
        NOSFIX_CHECK(collected[i].input.edge == reference[i].input.edge);
    }
}

void TestUnsupportedDescriptorFailsClosed() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'C', 7);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    PushKey(fixture, kAnchor + 10, 'C', 0x2E, 0, capture);

    constexpr std::uint64_t kBadHandle = 0x900;
    const std::uint64_t bad_token = fixture.registry.Add(DeviceSpec{});
    fixture.probe.Set(kBadHandle, bad_token, DeviceProbeStatus::DescriptorRejected);
    const auto payload = synthetic::HidPayload(kBadHandle, 3, {synthetic::Report(1, 3)});
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 20, payload, capture));

    Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.size() == 2); // press, then synthetic release
    if (run.events.size() == 2) {
        NOSFIX_CHECK(run.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(run.events[1].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[1].input.synthetic);
    }
    NOSFIX_CHECK(fixture.pipeline.Stats().unsupported_descriptors == 1);
    const std::uint64_t probes_after_first = fixture.probe.calls;

    // The failed handle is negatively cached: no second OS probe.
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 30, payload, capture));
    run = ProcessAll(fixture);
    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.empty());
    NOSFIX_CHECK(fixture.probe.calls == probes_after_first);
}

void TestReconnectGenerationAndStaleIdentity() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'D', 9);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);

    PushKey(fixture, kAnchor + 10, 'D', 0x20, 0, capture);
    PushBreak(fixture, kAnchor + 20, 'D', 0x20, capture);
    Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.events.size() == 2);
    if (run.events.size() == 2) NOSFIX_CHECK(run.events[0].input.device_generation == 1);

    RawInputBoundary removal;
    removal.reason = RawInputResetReason::DeviceChange;
    removal.device_change = RawInputDeviceChangeKind::Removal;
    removal.device_handle = kKeyboardHandle;
    fixture.queue.MarkGap(kAnchor + 30, removal, capture);
    run = ProcessAll(fixture);
    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.empty());
    std::array<DeviceSummary, kInputMaxDeviceCount> devices{};
    NOSFIX_CHECK(fixture.pipeline.Devices(devices) == 0);
    NOSFIX_CHECK(fixture.pipeline.Stats().device_removals == 1);

    // Reconnect: same source_id/control, new generation, binding reinstalled.
    PushKey(fixture, kAnchor + 40, 'D', 0x20, 0, capture);
    PushBreak(fixture, kAnchor + 45, 'D', 0x20, capture);
    run = ProcessAll(fixture);
    NOSFIX_CHECK(run.events.size() == 2);
    if (run.events.size() == 2) {
        NOSFIX_CHECK(run.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(run.events[0].input.device_generation == 2);
        NOSFIX_CHECK(run.events[1].input.edge == InputEdge::Release);
    }
    NOSFIX_CHECK(fixture.pipeline.Devices(devices) == 1);
    if (fixture.pipeline.Devices(devices) == 1) {
        NOSFIX_CHECK(devices[0].device_generation == 2);
        NOSFIX_CHECK(devices[0].binding_status == DeviceBindingStatus::Installed);
    }

    // Stale identity: a packet captured in epoch 2, still queued when epoch 3
    // is produced, and from a handle never resolved. It must not be decoded.
    constexpr std::uint64_t kStaleHandle = 0x300;
    const std::uint64_t stale_token = fixture.registry.Add(DeviceSpec{});
    fixture.probe.Set(kStaleHandle, stale_token);
    const auto stale_payload = synthetic::HidPayload(
        kStaleHandle, 3, {synthetic::Report(1, 3)});
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 50, stale_payload, capture));
    fixture.queue.MarkGap(kAnchor + 60, RawInputResetReason::QueueOverflow, capture);
    run = ProcessAll(fixture);
    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.empty());
    NOSFIX_CHECK(fixture.pipeline.Stats().stale_records >= 1);
    NOSFIX_CHECK(fixture.pipeline.Stats().device_failure_boundaries >= 1);
}

void TestBindingValidation() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);

    PipelineBindingSpec invalid;
    invalid.source_id = kKeyboardSource;
    invalid.control_id = MakeKeyboardControlId(static_cast<std::uint32_t>('A'));
    invalid.lane = kInputLaneCount;
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&invalid, 1}) ==
                 PipelineConfigStatus::RejectedInvalidBinding);

    invalid = KeyBinding(kKeyboardSource, 'A', 0);
    invalid.source_id = 0;
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&invalid, 1}) ==
                 PipelineConfigStatus::RejectedInvalidBinding);

    const std::array<PipelineBindingSpec, 2> duplicate{
        KeyBinding(kKeyboardSource, 'A', 1),
        KeyBinding(kKeyboardSource, 'A', 1),
    };
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings(duplicate) ==
                 PipelineConfigStatus::RejectedDuplicateBinding);

    std::array<PipelineBindingSpec, 2> inconsistent{
        KeyBinding(kKeyboardSource, 'B', 1),
        KeyBinding(kKeyboardSource, 'B', 2),
    };
    inconsistent[1].kind = InputControlKind::Value;
    inconsistent[1].press_threshold = 0.5f;
    inconsistent[1].release_threshold = 0.4f;
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings(inconsistent) ==
                 PipelineConfigStatus::RejectedInconsistentBinding);

    PipelineBindingSpec threshold_on_button = KeyBinding(kKeyboardSource, 'C', 2);
    threshold_on_button.press_threshold = 0.5f;
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&threshold_on_button, 1}) ==
                 PipelineConfigStatus::RejectedInvalidThreshold);

    PipelineBindingSpec bad_value;
    bad_value.source_id = kHidSource;
    bad_value.control_id = MakeHidButtonControlId(0);
    bad_value.kind = InputControlKind::Value;
    bad_value.press_threshold = 0.4f;
    bad_value.release_threshold = 0.6f; // release > press
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&bad_value, 1}) ==
                 PipelineConfigStatus::RejectedInvalidThreshold);

    DeviceSpec hid;
    hid.source_id = kHidSource;
    hid.input_report_bytes = 3;
    hid.buttons = {ButtonCapSpec{1, 1, 0, false, 1, 1, 0}};
    std::array<PipelineBindingSpec, 2> dormant{
        PipelineBindingSpec{kHidSource, MakeHidButtonControlId(0), 4},
        PipelineBindingSpec{kHidSource, MakeHidButtonControlId(5), 5},
    };
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&dormant[0], 1}) ==
                 PipelineConfigStatus::Accepted);
    const std::uint64_t token = fixture.AddHid(kHidHandle, hid);
    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    fixture.registry.Get(token).SetOn(1, 1, 0, {1});
    const auto payload = synthetic::HidPayload(kHidHandle, 3, {synthetic::Report(1, 3)});
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 10, payload, capture));
    Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.events.size() == 1);
    if (run.events.size() == 1) NOSFIX_CHECK(run.events[0].input.lane == 4);

    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&dormant[1], 1}) ==
                 PipelineConfigStatus::RejectedUnknownControl);

    fixture.registry.Get(token).SetOn(1, 1, 0, {});
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 20, payload, capture));
    run = ProcessAll(fixture);
    NOSFIX_CHECK(run.events.size() == 1);
    if (run.events.size() == 1) {
        NOSFIX_CHECK(run.events[0].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[0].input.lane == 4);
    }
}

void TestWatermark() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'E', 0);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    NOSFIX_CHECK(!fixture.pipeline.Watermark().cursor.valid);
    NOSFIX_CHECK(!fixture.pipeline.FullyProcessed());
    NOSFIX_CHECK(fixture.pipeline.ProducerCursor().valid == false);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    PushKey(fixture, kAnchor + 10, 'E', 0x12, 0, capture);

    std::array<PipelineInputEvent, 8> buffer{};
    const PipelineProcessResult first = fixture.pipeline.ProcessOne(buffer);
    NOSFIX_CHECK(first.step == PipelineStep::Processed);
    auto watermark = fixture.pipeline.Watermark();
    NOSFIX_CHECK(watermark.cursor.valid && watermark.cursor.sequence == 1);
    NOSFIX_CHECK(watermark.song.valid() && watermark.song.sequence == 1);
    NOSFIX_CHECK(std::abs(watermark.song.time.microseconds - 1.0) < 1e-6);
    NOSFIX_CHECK(fixture.pipeline.FullyProcessed());
    NOSFIX_CHECK(fixture.pipeline.ProducerCursor().sequence == 1);

    const auto unchanged = fixture.pipeline.ProcessOne(buffer);
    NOSFIX_CHECK(unchanged.step == PipelineStep::NoRecord);
    NOSFIX_CHECK(fixture.pipeline.Watermark().cursor.sequence == 1);

    // A later observation must not move the processed watermark; a packet
    // captured before it must still use its own segment.
    NOSFIX_CHECK(clock.Seek(kAnchor + 1'000, 9'000'000.0));
    NOSFIX_CHECK(fixture.pipeline.Watermark().song.time.microseconds < 1'000.0);
    PushBreak(fixture, kAnchor + 20, 'E', 0x12, capture);
    NOSFIX_CHECK(fixture.pipeline.ProducerCursor().sequence == 2);
    NOSFIX_CHECK(!fixture.pipeline.FullyProcessed());
    const PipelineProcessResult second = fixture.pipeline.ProcessOne(buffer);
    NOSFIX_CHECK(second.step == PipelineStep::Processed);
    NOSFIX_CHECK(fixture.pipeline.FullyProcessed());
    NOSFIX_CHECK(std::abs(fixture.pipeline.Watermark().song.time.microseconds - 2.0) < 1e-6);
}

void TestStaleAndOverflowBoundaries() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'F', 1);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    // Enroll and press first: a packet from a *never resolved* handle that is
    // still queued when a stream boundary occurs is dropped as stale (see
    // TestReconnectGenerationAndStaleIdentity); the device must be known.
    PushKey(fixture, kAnchor + 10, 'F', 0x21, 0, capture);
    Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.events.size() == 1);
    fixture.queue.MarkGap(kAnchor + 20, RawInputResetReason::QueueOverflow, capture);
    PushKey(fixture, kAnchor + 30, 'F', 0x21, 0, capture);
    fixture.queue.MarkGap(kAnchor + 40, RawInputResetReason::ReadFailure, capture);
    Append(run, ProcessAll(fixture));

    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.size() == 4);
    if (run.events.size() == 4) {
        NOSFIX_CHECK(run.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(run.events[1].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[1].input.synthetic);
        NOSFIX_CHECK(run.events[2].input.edge == InputEdge::Press);
        NOSFIX_CHECK(run.events[3].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[3].input.synthetic);
    }
    NOSFIX_CHECK(fixture.pipeline.Stats().reset_records == 2);
    NOSFIX_CHECK(fixture.pipeline.Stats().faults == 0);
}

void TestBacklogOrdering() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'G', 2);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    constexpr std::size_t kEdges = 200;
    for (std::size_t i = 0; i < kEdges; ++i) {
        const USHORT flags = (i & 1U) ? RI_KEY_BREAK : 0;
        PushKey(fixture, kAnchor + 10 + static_cast<std::int64_t>(i), 'G', 0x22,
                flags, capture);
    }
    const Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.size() == kEdges);
    for (std::size_t i = 0; i < run.events.size() && i < kEdges; ++i) {
        NOSFIX_CHECK(run.events[i].input.stamp.sequence == i + 1);
        NOSFIX_CHECK(run.events[i].input.edge ==
                     ((i & 1U) ? InputEdge::Release : InputEdge::Press));
    }
    NOSFIX_CHECK(fixture.pipeline.Stats().faults == 0);
}

void TestUnknownReportIdFailsClosed() {
    Fixture fixture;
    DeviceSpec spec;
    spec.source_id = kHidSource;
    spec.input_report_bytes = 3;
    spec.buttons = {ButtonCapSpec{1, 1, 0, false, 1, 1, 0}};
    const std::uint64_t token = fixture.AddHid(kHidHandle, spec);
    const PipelineBindingSpec binding{kHidSource, MakeHidButtonControlId(0), 0};
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    Run run;
    fixture.registry.Get(token).SetOn(1, 1, 0, {1});
    auto payload = synthetic::HidPayload(kHidHandle, 3, {synthetic::Report(1, 3)});
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 10, payload, capture));
    Append(run, ProcessAll(fixture));
    payload = synthetic::HidPayload(kHidHandle, 3, {synthetic::Report(9, 3)});
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 20, payload, capture));
    Append(run, ProcessAll(fixture));

    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.size() == 2);
    if (run.events.size() == 2) {
        NOSFIX_CHECK(run.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(run.events[1].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[1].input.synthetic);
    }
    NOSFIX_CHECK(fixture.pipeline.Stats().unsupported_reports == 1);
}

void TestNoAllocationOnProcessPath() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'H', 0);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    const auto down = synthetic::KeyboardPayload(kKeyboardHandle, 'H', 0x23, 0);
    const auto up = synthetic::KeyboardPayload(kKeyboardHandle, 'H', 0x23, RI_KEY_BREAK);

    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 10, down, capture));
    std::array<PipelineInputEvent, 8> buffer{};
    (void)fixture.pipeline.ProcessOne(buffer);
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 20, up, capture));
    (void)fixture.pipeline.ProcessOne(buffer);
    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 30, down, capture));

    allocation_probe::tracking = true;
    allocation_probe::count = 0;
    const PipelineProcessResult result = fixture.pipeline.ProcessOne(buffer);
    allocation_probe::tracking = false;
    NOSFIX_CHECK(result.step == PipelineStep::Processed);
    NOSFIX_CHECK(result.events_written == 1);
    NOSFIX_CHECK(allocation_probe::count == 0);
}

void TestFenceWatermark() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'J', 0);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    PushKey(fixture, kAnchor + 10, 'J', 0x1F, 0, capture);
    Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.events.size() == 1);
    auto watermark = fixture.pipeline.Watermark();
    NOSFIX_CHECK(watermark.cursor.valid && watermark.cursor.qpc_exact);
    NOSFIX_CHECK(watermark.cursor.sequence == 1);
    NOSFIX_CHECK(watermark.song.valid());

    // An idle keyboard: a producer fence advances processed-through time with
    // no key event and no normalizer involvement.
    NOSFIX_CHECK(fixture.queue.PushFence(kAnchor + 1'000, Capture(clock), true));
    run = ProcessAll(fixture);
    NOSFIX_CHECK(run.ok);
    NOSFIX_CHECK(run.events.empty());
    watermark = fixture.pipeline.Watermark();
    NOSFIX_CHECK(watermark.cursor.sequence == 2);
    NOSFIX_CHECK(watermark.cursor.qpc == kAnchor + 1'000);
    NOSFIX_CHECK(watermark.cursor.qpc_exact);
    NOSFIX_CHECK(watermark.song.valid());
    NOSFIX_CHECK(std::abs(watermark.song.time.microseconds - 100.0) < 1e-6);
    NOSFIX_CHECK(fixture.pipeline.Stats().fence_records == 1);

    NOSFIX_CHECK(fixture.queue.PushFence(kAnchor + 2'000, Capture(clock), true));
    run = ProcessAll(fixture);
    NOSFIX_CHECK(run.events.empty());
    watermark = fixture.pipeline.Watermark();
    NOSFIX_CHECK(watermark.cursor.sequence == 3);
    NOSFIX_CHECK(watermark.cursor.qpc == kAnchor + 2'000);
    NOSFIX_CHECK(std::abs(watermark.song.time.microseconds - 200.0) < 1e-6);
    NOSFIX_CHECK(fixture.pipeline.FullyProcessed());
}

void TestQpcReversal() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'K', 0);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    const auto down = synthetic::KeyboardPayload(kKeyboardHandle, 'K', 0x25, 0);
    const auto up = synthetic::KeyboardPayload(kKeyboardHandle, 'K', 0x25, RI_KEY_BREAK);

    NOSFIX_CHECK(fixture.queue.PushRawInput(kAnchor + 100, down, capture, true));
    Run run = ProcessAll(fixture);
    NOSFIX_CHECK(run.events.size() == 1);
    NOSFIX_CHECK(!fixture.queue.PushRawInput(kAnchor + 50, up, capture, true));
    NOSFIX_CHECK(fixture.queue.Stats().clock_reversals == 1);
    Append(run, ProcessAll(fixture));
    NOSFIX_CHECK(run.events.size() == 2); // press + synthetic release
    if (run.events.size() == 2) {
        NOSFIX_CHECK(run.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(run.events[1].input.edge == InputEdge::Release);
        NOSFIX_CHECK(run.events[1].input.synthetic);
    }
    const auto stats = fixture.pipeline.Stats();
    NOSFIX_CHECK(stats.qpc_regressions >= 1);
    NOSFIX_CHECK(stats.last_reset_reason == RawInputResetReason::ClockReversal);
    NOSFIX_CHECK(stats.last_rejected_qpc == kAnchor + 50);
    const auto watermark = fixture.pipeline.Watermark();
    NOSFIX_CHECK(watermark.cursor.sequence == 2);
    NOSFIX_CHECK(!watermark.cursor.qpc_exact);
    NOSFIX_CHECK(watermark.cursor.qpc == kAnchor + 100);
    NOSFIX_CHECK(watermark.song.status == SongWatermarkStatus::InvalidatedStreamLoss);

    // A reversed fence is refused the same way.
    NOSFIX_CHECK(!fixture.queue.PushFence(kAnchor + 60, capture, true));
    NOSFIX_CHECK(fixture.queue.Stats().clock_reversals == 2);
}

void TestProducerCursorCoherence() {
    RawInputPacketQueue queue;
    NOSFIX_CHECK(queue.Ready());
    constexpr std::int64_t kBase = 5'000'000'000LL;
    constexpr std::size_t kCount = 4'000;
    std::atomic<bool> done{false};
    std::atomic<bool> failed{false};
    std::thread producer([&] {
        for (std::size_t i = 0; i < kCount; ++i) {
            const auto payload = synthetic::KeyboardPayload(
                kKeyboardHandle, 'A', 0x1E, (i & 1U) ? RI_KEY_BREAK : 0);
            if (!queue.PushRawInput(kBase + static_cast<std::int64_t>(i + 1) * 10,
                                    payload, {}, true)) {
                failed.store(true, std::memory_order_release);
            }
        }
        done.store(true, std::memory_order_release);
    });

    std::uint64_t last_sequence = 0;
    std::uint64_t iterations = 0;
    while (!done.load(std::memory_order_acquire)) {
        const auto cursor = queue.ProducerCursor();
        if (cursor.valid) {
            // A coherent (qpc, sequence) pair: a torn mix from two publications
            // would break this exact relation.
            NOSFIX_CHECK(cursor.qpc_exact);
            NOSFIX_CHECK(cursor.qpc == kBase +
                         static_cast<std::int64_t>(cursor.sequence) * 10);
            NOSFIX_CHECK(cursor.sequence >= last_sequence);
            last_sequence = cursor.sequence;
        }
        if (++iterations > 20'000'000) break;
        std::this_thread::yield();
    }
    producer.join();
    const auto final_cursor = queue.ProducerCursor();
    NOSFIX_CHECK(final_cursor.valid);
    NOSFIX_CHECK(final_cursor.qpc_exact);
    NOSFIX_CHECK(final_cursor.sequence == kCount);
    NOSFIX_CHECK(final_cursor.qpc == kBase + static_cast<std::int64_t>(kCount) * 10);
    NOSFIX_CHECK(!failed.load(std::memory_order_acquire));
    NOSFIX_CHECK(queue.Stats().clock_reversals == 0);
}

void TestSongWatermarkRegression() {
    // (a) An observed backwards mapping within one generation fails the song
    // watermark closed for the rest of the generation.
    {
        Fixture fixture;
        fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
        const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'L', 0);
        NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                     PipelineConfigStatus::Accepted);
        SongClock clock;
        NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
        const auto before = Capture(clock);
        PushKey(fixture, kAnchor + 1'000, 'L', 0x26, 0, before); // 100 us
        NOSFIX_CHECK(ProcessAll(fixture).events.size() == 1);
        auto watermark = fixture.pipeline.Watermark();
        NOSFIX_CHECK(watermark.song.valid());
        NOSFIX_CHECK(std::abs(watermark.song.time.microseconds - 100.0) < 1e-6);

        // Same generation, negative correction: A+2000 maps to 0 us, so A+2500
        // maps to 50 us, earlier than the 100 us already processed.
        NOSFIX_CHECK(clock.Observe(kAnchor + 2'000, kFrequency, 0.0, 1'000.0));
        const auto after = Capture(clock);
        PushBreak(fixture, kAnchor + 2'500, 'L', 0x26, after);
        NOSFIX_CHECK(ProcessAll(fixture).events.size() == 1);
        watermark = fixture.pipeline.Watermark();
        NOSFIX_CHECK(!watermark.song.valid());
        NOSFIX_CHECK(watermark.song.status == SongWatermarkStatus::InvalidatedRegression);
        NOSFIX_CHECK(fixture.pipeline.Stats().song_watermark_breaks >= 1);

        // Further monotonic records in the same generation stay uncertified.
        PushKey(fixture, kAnchor + 2'600, 'L', 0x26, 0, after);
        NOSFIX_CHECK(ProcessAll(fixture).events.size() == 1);
        NOSFIX_CHECK(fixture.pipeline.Watermark().song.status ==
                     SongWatermarkStatus::InvalidatedRegression);
    }

    // (b) A publication explicitly flagged PossiblyRegressed invalidates even
    // when the observed time did not go backwards.
    {
        Fixture fixture;
        fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
        const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'M', 0);
        NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                     PipelineConfigStatus::Accepted);
        SongClock clock;
        NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
        const auto before = Capture(clock);
        PushKey(fixture, kAnchor + 1'000, 'M', 0x27, 0, before);
        NOSFIX_CHECK(ProcessAll(fixture).events.size() == 1);
        NOSFIX_CHECK(fixture.pipeline.Watermark().song.valid());
        const auto regressed = CaptureRegressed(clock);
        PushBreak(fixture, kAnchor + 2'000, 'M', 0x27, regressed);
        NOSFIX_CHECK(ProcessAll(fixture).events.size() == 1);
        const auto watermark = fixture.pipeline.Watermark();
        NOSFIX_CHECK(!watermark.song.valid());
        NOSFIX_CHECK(watermark.song.status == SongWatermarkStatus::InvalidatedRegression);
    }

    // (c) A generation change invalidates the old watermark and establishes the
    // new generation from the record that carries it.
    {
        Fixture fixture;
        fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
        const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'N', 0);
        NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                     PipelineConfigStatus::Accepted);
        SongClock clock;
        NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
        const auto old_capture = Capture(clock);
        PushKey(fixture, kAnchor + 10, 'N', 0x28, 0, old_capture);
        NOSFIX_CHECK(ProcessAll(fixture).events.size() == 1);
        NOSFIX_CHECK(fixture.pipeline.Watermark().song.valid());

        NOSFIX_CHECK(clock.Seek(kAnchor + 1'000, 9'000'000.0));
        const auto new_capture = Capture(clock);
        // A packet captured before the seek but processed after it: it keeps
        // its own old generation (never re-labelled to the new one).
        PushBreak(fixture, kAnchor + 20, 'N', 0x28, old_capture);
        PushKey(fixture, kAnchor + 1'010, 'N', 0x28, 0, new_capture);
        const Run run = ProcessAll(fixture);
        NOSFIX_CHECK(run.events.size() == 2);
        if (run.events.size() == 2) {
            NOSFIX_CHECK(run.events[0].song_time.generation == 1);
            NOSFIX_CHECK(std::abs(run.events[0].song_time.microseconds - 2.0) < 1e-6);
            NOSFIX_CHECK(run.events[1].song_time.generation == 2);
            NOSFIX_CHECK(run.events[1].song_time.microseconds > 9'000'000.0);
        }
        const auto watermark = fixture.pipeline.Watermark();
        NOSFIX_CHECK(watermark.song.valid());
        NOSFIX_CHECK(watermark.song.time.generation == 2);
        NOSFIX_CHECK(fixture.pipeline.Stats().generation_changes >= 1);
        NOSFIX_CHECK(fixture.pipeline.Stats().song_watermark_breaks >= 1);
    }
}

void TestStreamLossInvalidatesWatermark() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'P', 0);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    PushKey(fixture, kAnchor + 10, 'P', 0x19, 0, capture);
    NOSFIX_CHECK(ProcessAll(fixture).events.size() == 1);
    NOSFIX_CHECK(fixture.pipeline.Watermark().song.valid());

    fixture.queue.MarkGap(kAnchor + 20, RawInputResetReason::DeviceChange, capture);
    const Run loss = ProcessAll(fixture);
    NOSFIX_CHECK(loss.events.size() == 1); // synthetic release
    auto watermark = fixture.pipeline.Watermark();
    NOSFIX_CHECK(watermark.cursor.sequence == 2);
    NOSFIX_CHECK(watermark.song.status == SongWatermarkStatus::InvalidatedStreamLoss);
    NOSFIX_CHECK(!watermark.song.valid());

    // A later explicitly timed record re-establishes the new baseline.
    NOSFIX_CHECK(fixture.queue.PushFence(kAnchor + 1'000, Capture(clock), true));
    NOSFIX_CHECK(ProcessAll(fixture).events.empty());
    watermark = fixture.pipeline.Watermark();
    NOSFIX_CHECK(watermark.song.valid());
    NOSFIX_CHECK(watermark.cursor.sequence == 3);
    NOSFIX_CHECK(std::abs(watermark.song.time.microseconds - 100.0) < 1e-6);
}

void TestAnalogLevelState() {
    Fixture fixture;
    DeviceSpec spec;
    spec.source_id = kHidSource;
    spec.input_report_bytes = 3;
    spec.values = {ValueCapSpec{1, 0, 0, false, 0x30, 0x30, 0, 0, 255, 8, false}};
    const std::uint64_t token = fixture.AddHid(kHidHandle, spec);
    PipelineBindingSpec binding;
    binding.source_id = kHidSource;
    binding.control_id = MakeHidValueControlId(0);
    binding.lane = 0;
    binding.kind = InputControlKind::Value;
    binding.press_threshold = 0.6f;
    binding.release_threshold = 0.4f;
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const auto capture = Capture(clock);
    Run run;
    auto push = [&](ULONG raw, std::int64_t qpc) {
        fixture.registry.Get(token).SetRaw(0, 1, 0, 0x30, raw, false);
        const auto payload = synthetic::HidPayload(
            kHidHandle, 3, {synthetic::Report(0, 3)});
        NOSFIX_CHECK(fixture.queue.PushRawInput(qpc, payload, capture));
        Append(run, ProcessAll(fixture));
    };

    push(204, kAnchor + 10); // 0.8: press
    NOSFIX_CHECK(run.events.size() == 1);
    if (run.events.size() == 1) {
        NOSFIX_CHECK(run.events[0].input.edge == InputEdge::Press);
        NOSFIX_CHECK(std::abs(run.events[0].input.velocity - 0.8f) < 1e-6f);
    }
    std::array<AnalogLevelView, kInputLaneCount> levels{};
    NOSFIX_CHECK(fixture.pipeline.AnalogLevels(levels) == kInputLaneCount);
    NOSFIX_CHECK(levels[0].valid);
    NOSFIX_CHECK(std::abs(levels[0].level - 0.8f) < 1e-6f);
    NOSFIX_CHECK(levels[0].panb_velocity == 12); // 0.8 * 15.999 = 12.799 -> 12
    NOSFIX_CHECK(levels[0].stamp.sequence == 1);

    // Held pressure changes without a digital edge are still observable.
    push(128, kAnchor + 20); // 0.5019, still above the 0.4 release threshold
    NOSFIX_CHECK(run.events.size() == 1);
    NOSFIX_CHECK(fixture.pipeline.AnalogLevels(levels) == kInputLaneCount);
    NOSFIX_CHECK(std::abs(levels[0].level - (128.0f / 255.0f)) < 1e-6f);
    NOSFIX_CHECK(levels[0].panb_velocity == 8);
    NOSFIX_CHECK(levels[0].stamp.sequence == 2);

    push(51, kAnchor + 30); // 0.2: release
    NOSFIX_CHECK(run.events.size() == 2);
    if (run.events.size() == 2) {
        NOSFIX_CHECK(run.events[1].input.edge == InputEdge::Release);
    }
    NOSFIX_CHECK(fixture.pipeline.AnalogLevels(levels) == kInputLaneCount);
    NOSFIX_CHECK(levels[0].level == 0.0f);
    NOSFIX_CHECK(levels[0].panb_velocity == 0);
    NOSFIX_CHECK(fixture.pipeline.Stats().analog_observations == 3);
}

void TestPublicationRace() {
    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const SongClockSegment first = clock.Snapshot();
    NOSFIX_CHECK(clock.Seek(kAnchor + 10, 10'000'000.0));
    const SongClockSegment second = clock.Snapshot();

    SongClockPublication publication;
    NOSFIX_CHECK(!publication.HasPublished());
    SongClockSnapshot read{};
    NOSFIX_CHECK(!publication.TryRead(read));
    publication.Publish(first, SongClockContinuity::Monotonic);
    NOSFIX_CHECK(publication.HasPublished());
    NOSFIX_CHECK(publication.TryRead(read));
    const auto mapped = read.segment.Map(kAnchor + 5);
    NOSFIX_CHECK(mapped.has_value());
    if (mapped) NOSFIX_CHECK(std::abs(mapped->microseconds - 0.5) < 1e-6);

    std::atomic<bool> writing{true};
    std::thread writer([&] {
        for (unsigned i = 0; i < 200'000; ++i) {
            publication.Publish((i & 1U) ? first : second, SongClockContinuity::Monotonic);
        }
        writing.store(false, std::memory_order_release);
    });

    std::uint64_t reads = 0;
    std::uint64_t last_version = 0;
    while (writing.load(std::memory_order_acquire) || reads < 400'000) {
        SongClockSnapshot snapshot{};
        if (!publication.TryRead(snapshot)) {
            std::this_thread::yield();
            continue;
        }
        ++reads;
        // Coherent fields: versions are non-decreasing and match the claim.
        NOSFIX_CHECK(snapshot.version >= last_version);
        if (snapshot.version < last_version) break;
        last_version = snapshot.version;
        NOSFIX_CHECK(snapshot.continuity == SongClockContinuity::Monotonic);
        const auto sample = snapshot.segment.Map(kAnchor + 100);
        NOSFIX_CHECK(sample.has_value());
        if (!sample) break;
        if (sample->generation == 1) {
            NOSFIX_CHECK(sample->microseconds < 1'000.0);
        } else if (sample->generation == 2) {
            NOSFIX_CHECK(sample->microseconds > 9'000'000.0);
        } else {
            NOSFIX_CHECK(false);
        }
        if (g_failures != 0) break;
        if (reads > 2'000'000) break;
    }
    writer.join();
    NOSFIX_CHECK(reads > 0);
}

void TestThreadedCapturePipeline() {
    Fixture fixture;
    fixture.AddKeyboard(kKeyboardHandle, kKeyboardSource);
    const PipelineBindingSpec binding = KeyBinding(kKeyboardSource, 'I', 0);
    NOSFIX_CHECK(fixture.pipeline.ConfigureBindings({&binding, 1}) ==
                 PipelineConfigStatus::Accepted);

    SongClock clock;
    NOSFIX_CHECK(clock.Start(kAnchor, kFrequency, 0.0));
    const SongClockSegment first = clock.Snapshot();
    NOSFIX_CHECK(clock.Seek(kAnchor + 10, 10'000'000.0));
    const SongClockSegment second = clock.Snapshot();

    SongClockPublication publication;
    publication.Publish(first, SongClockContinuity::Monotonic);
    std::atomic<bool> stop_writer{false};
    std::thread writer([&] {
        for (unsigned i = 0; !stop_writer.load(std::memory_order_acquire); ++i) {
            publication.Publish((i & 1U) ? first : second, SongClockContinuity::Monotonic);
        }
    });

    constexpr std::size_t kEdges = 4'000;
    std::thread producer([&] {
        for (std::size_t i = 0; i < kEdges; ++i) {
            RawInputCaptureClock capture;
            SongClockSnapshot snapshot;
            if (publication.TryRead(snapshot)) {
                capture.segment = snapshot.segment;
                capture.version = snapshot.version;
                capture.continuity = snapshot.continuity;
                capture.valid = true;
            }
            const USHORT flags = (i & 1U) ? RI_KEY_BREAK : 0;
            const auto payload = synthetic::KeyboardPayload(
                kKeyboardHandle, 'I', 0x24, flags);
            if (!fixture.queue.PushRawInput(kAnchor + 100 + static_cast<std::int64_t>(i),
                                            payload, capture)) {
                std::this_thread::yield();
                --i;
                continue;
            }
            if ((i & 63U) == 0) std::this_thread::yield();
        }
    });

    std::vector<PipelineInputEvent> events;
    std::array<PipelineInputEvent, 32> buffer{};
    for (;;) {
        const PipelineProcessResult result = fixture.pipeline.ProcessOne(buffer);
        events.insert(events.end(), buffer.begin(),
                      buffer.begin() + static_cast<std::ptrdiff_t>(result.events_written));
        if (result.step == PipelineStep::Processed ||
            result.step == PipelineStep::NeedOutputCapacity) {
            continue;
        }
        if (result.step == PipelineStep::Faulted) break;
        if (events.size() >= kEdges) break;
        std::this_thread::yield();
    }
    producer.join();
    stop_writer.store(true, std::memory_order_release);
    writer.join();

    for (;;) {
        const PipelineProcessResult result = fixture.pipeline.ProcessOne(buffer);
        events.insert(events.end(), buffer.begin(),
                      buffer.begin() + static_cast<std::ptrdiff_t>(result.events_written));
        if (result.step != PipelineStep::Processed &&
            result.step != PipelineStep::NeedOutputCapacity) {
            break;
        }
    }

    NOSFIX_CHECK(events.size() == kEdges);
    NOSFIX_CHECK(fixture.pipeline.Stats().faults == 0);
    for (std::size_t i = 0; i < events.size() && i < kEdges; ++i) {
        NOSFIX_CHECK(events[i].input.stamp.sequence == i + 1);
        NOSFIX_CHECK(events[i].input.edge ==
                     ((i & 1U) ? InputEdge::Release : InputEdge::Press));
        if (!events[i].has_song_time) continue;
        if (events[i].song_time.generation == 1) {
            NOSFIX_CHECK(events[i].song_time.microseconds < 1'000.0);
        } else if (events[i].song_time.generation == 2) {
            NOSFIX_CHECK(events[i].song_time.microseconds > 9'000'000.0);
        } else {
            NOSFIX_CHECK(false);
        }
    }
}

} // namespace

int main() {
    TestRapidTapsAndImmutableMapping();
    TestTypematicAndReset();
    TestChordsAndAlternatives();
    TestPausedPlayback();
    TestReportIdIsolation();
    TestOutputCapacityRetry();
    TestUnsupportedDescriptorFailsClosed();
    TestReconnectGenerationAndStaleIdentity();
    TestBindingValidation();
    TestWatermark();
    TestStaleAndOverflowBoundaries();
    TestBacklogOrdering();
    TestUnknownReportIdFailsClosed();
    TestNoAllocationOnProcessPath();
    TestAnalogLevelState();
    TestFenceWatermark();
    TestQpcReversal();
    TestProducerCursorCoherence();
    TestSongWatermarkRegression();
    TestStreamLossInvalidatesWatermark();
    TestPublicationRace();
    TestThreadedCapturePipeline();
    std::fprintf(stderr, "input_pipeline: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
