#include "test_support.hpp"

#include "nosfix/audio_mixer.hpp"
#include "nosfix/keysound_dispatch.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <thread>

namespace {

using nosfix::audio::KeysoundDispatch;
using nosfix::audio::KeysoundDispatchOutcome;
using nosfix::audio::KeysoundPushStatus;
using nosfix::audio::KeysoundQueue;
using nosfix::audio::KeysoundRequest;

constexpr std::int64_t kQpcOrigin = 5'000'000;
constexpr std::int64_t kQpcFrequency = 10'000'000;
constexpr std::uint32_t kRate = 44100;

struct Mapper {
    bool valid = true;
};

bool MapQpc(void* context, std::int64_t qpc, std::int64_t& sample) noexcept {
    auto* mapper = static_cast<Mapper*>(context);
    if (!mapper->valid) return false;
    const long double delta = static_cast<long double>(qpc - kQpcOrigin);
    const long double value =
        delta * static_cast<long double>(kRate) / static_cast<long double>(kQpcFrequency);
    sample = static_cast<std::int64_t>(std::llround(value));
    return true;
}

struct Sink {
    KeysoundDispatch items[512]{};
    std::uint32_t count = 0;
    bool reject = false;
    std::uint32_t rejected = 0;
};

bool Receive(void* context, const KeysoundDispatch& dispatch) noexcept {
    auto* sink = static_cast<Sink*>(context);
    if (sink->reject) {
        ++sink->rejected;
        return false;
    }
    if (sink->count < 512) sink->items[sink->count++] = dispatch;
    return true;
}

KeysoundRequest Request(std::uint64_t sequence, std::uint64_t generation,
                        std::int64_t capture_qpc, std::uint32_t cue = 1) {
    KeysoundRequest request;
    request.sequence = sequence;
    request.generation = generation;
    request.capture_qpc = capture_qpc;
    request.cue_id = cue;
    request.variation = 0;
    request.volume_db = -3.0F;
    request.pan = 0.25F;
    return request;
}

std::int64_t SampleAtQpc(std::int64_t qpc) noexcept {
    Mapper mapper;
    std::int64_t sample = 0;
    MapQpc(&mapper, qpc, sample);
    return sample;
}

constexpr std::uint32_t kStressEvents = 4'000;

bool StressMap(void* context, std::int64_t qpc, std::int64_t& sample) noexcept {
    (void)context;
    sample = qpc - kQpcOrigin;
    return true;
}

bool MinimumMap(void* context, std::int64_t qpc, std::int64_t& sample) noexcept {
    (void)context;
    (void)qpc;
    sample = std::numeric_limits<std::int64_t>::min();
    return true;
}

struct StressSink {
    std::array<std::atomic<std::uint32_t>, kStressEvents + 1> seen{};
    std::atomic<std::uint32_t> duplicates{0};
    std::atomic<std::uint32_t> count{0};
};

bool StressReceive(void* context, const KeysoundDispatch& dispatch) noexcept {
    auto* sink = static_cast<StressSink*>(context);
    if (dispatch.sequence == 0 || dispatch.sequence > kStressEvents) {
        sink->duplicates.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    if (sink->seen[dispatch.sequence].exchange(1, std::memory_order_acq_rel) != 0) {
        sink->duplicates.fetch_add(1, std::memory_order_relaxed);
    }
    sink->count.fetch_add(1, std::memory_order_relaxed);
    return true;
}

// ---------------------------------------------------------------------------
// Frame-gated cue array (round 2 model of nostalgia sub_180304F30)
// ---------------------------------------------------------------------------

using nosfix::audio::FrameGateDecision;
using nosfix::audio::FrameGateOutcome;
using nosfix::audio::FrameGateReady;
using nosfix::audio::FrameGateRecord;
using nosfix::audio::FrameGateRequest;
using nosfix::audio::FrameGateSchedule;

FrameGateRequest GateRequest(std::int32_t id, std::uint64_t handle,
                             std::uint8_t param, std::int32_t defer) {
    FrameGateRequest request;
    request.sound_id = id;
    request.handle = handle;
    request.param = param;
    request.param2 = 3;
    request.defer_frames = defer;
    return request;
}

void TestFrameGateLayout() {
    using namespace nosfix::audio;
    CHECK(sizeof(FrameGateRecord) == 24);
    CHECK(offsetof(FrameGateRecord, sound_id) == 0);
    CHECK(offsetof(FrameGateRecord, param) == 5);
    CHECK(offsetof(FrameGateRecord, param2) == 6);
    CHECK(offsetof(FrameGateRecord, start_frame) == 8);
    CHECK(offsetof(FrameGateRecord, end_frame) == 12);
    CHECK(offsetof(FrameGateRecord, handle) == 16);
    // (0x1813E56D8 - 0x1813E4AD8) / 24 = 0xC00 / 24 = 128, which is the dedupe
    // bound at 0x1803050DB as well.
    CHECK(kFrameGateCapacity == 128);
    CHECK(kFrameGateRecordBytes == 24);
    CHECK(kFrameGateNoFrame == 0xFFFFFFFFu);
}

void TestFrameGateDeferred() {
    using namespace nosfix::audio;
    FrameGateSchedule gate;
    CHECK(!gate.Enabled());
    // word_1813E472C clear: the original function returns immediately.
    const FrameGateOutcome refused = gate.Schedule(GateRequest(7, 100, 5, 0), 100);
    CHECK(refused.decision == FrameGateDecision::Refused);
    CHECK(gate.Stats().refused_disabled == 1);
    CHECK(gate.LiveCount() == 0);
    CHECK(gate.RecordAt(0).sound_id == -1);

    gate.SetEnabled(true);
    CHECK(gate.Enabled());
    const FrameGateOutcome stored = gate.Schedule(GateRequest(7, 100, 5, 5), 100);
    CHECK(stored.decision == FrameGateDecision::Deferred);
    CHECK(stored.slot == 0);
    CHECK(stored.start_frame == 105); // current + a6
    CHECK(!stored.replaced_existing);
    CHECK(gate.LiveCount() == 1);
    CHECK(gate.Stats().deferred == 1);
    const FrameGateRecord& record = gate.RecordAt(0);
    CHECK(record.sound_id == 7);
    CHECK(record.handle == 100);
    CHECK(record.param == 5);
    CHECK(record.param2 == 3);
    CHECK(record.start_frame == 105);
    CHECK(record.end_frame == kFrameGateNoFrame);

    FrameGateReady ready[4]{};
    CHECK(gate.RetireDue(104, ready, 4) == 0);
    CHECK(gate.LiveCount() == 1);
    CHECK(gate.RetireDue(105, ready, 4) == 1);
    CHECK(ready[0].slot == 0);
    CHECK(ready[0].sound_id == 7);
    CHECK(ready[0].handle == 100);
    CHECK(ready[0].start_frame == 105);
    CHECK(gate.RecordAt(0).sound_id == -1);
    CHECK(gate.RecordAt(0).handle == 0);
    CHECK(gate.RecordAt(0).start_frame == kFrameGateNoFrame);
    CHECK(gate.LiveCount() == 0);
    CHECK(gate.Stats().retired == 1);
    CHECK(gate.RetireDue(1000, ready, 4) == 0); // never fires twice

    // a5 is added to the start frame when it is not -1.
    FrameGateRequest with_end = GateRequest(8, 101, 6, 10);
    with_end.extra = 20;
    const FrameGateOutcome end_outcome = gate.Schedule(with_end, 200);
    CHECK(end_outcome.decision == FrameGateDecision::Deferred);
    CHECK(end_outcome.start_frame == 210);
    CHECK(end_outcome.end_frame == 230);
    CHECK(gate.RecordAt(0).end_frame == 230);

    // a6 == 0 with a positive frame counter still defers to that frame: the
    // original tests `a6 + current > 0`, not `a6 > 0`.
    CHECK(gate.Schedule(GateRequest(9, 102, 7, 0), 300).decision ==
          FrameGateDecision::Deferred);
    CHECK(gate.RecordAt(1).start_frame == 300);
}

void TestFrameGateImmediate() {
    using namespace nosfix::audio;
    FrameGateSchedule gate;
    gate.SetEnabled(true);
    // The immediate path is `current + a6 <= 0` only.
    const FrameGateOutcome immediate = gate.Schedule(GateRequest(11, 200, 5, -100), 100);
    CHECK(immediate.decision == FrameGateDecision::ImmediateRecorded);
    CHECK(gate.Stats().immediate == 1);
    CHECK(gate.RecordAt(0).sound_id == 11);
    CHECK(gate.RecordAt(0).start_frame == kFrameGateNoFrame); // "already started"
    FrameGateReady ready[4]{};
    CHECK(gate.RetireDue(0xFFFFFFF0u, ready, 4) == 0); // the marker never fires
    CHECK(gate.LiveCount() == 1);

    // Dedupe: a stored pending record with the same a3 byte and the same handle
    // is found and its sound is stopped (sub_1802BCAD0), and it is counted.
    FrameGateSchedule dedupe;
    dedupe.SetEnabled(true);
    CHECK(dedupe.Schedule(GateRequest(21, 900, 5, 10), 100).decision ==
          FrameGateDecision::Deferred); // start 110
    const FrameGateOutcome hit = dedupe.Schedule(GateRequest(22, 900, 5, -100), 100);
    CHECK(hit.decision == FrameGateDecision::ImmediateRecorded);
    CHECK(hit.dedupe_stopped);
    CHECK(dedupe.Stats().dedupe_stopped == 1);
    // A different handle, a param outside 1..0x58 and an already-started marker
    // do not match.
    CHECK(dedupe.Schedule(GateRequest(23, 901, 5, -100), 100).dedupe_stopped == false);
    CHECK(dedupe.Schedule(GateRequest(24, 900, 0, -100), 100).dedupe_stopped == false);
    CHECK(dedupe.Schedule(GateRequest(25, 900, 0x59, -100), 100).dedupe_stopped == false);
    CHECK(dedupe.Stats().dedupe_stopped == 1);

    // Replacement: the same id is faded out and cleared before the new record is
    // written, so the slot is reused and only one record exists.
    FrameGateSchedule replace;
    replace.SetEnabled(true);
    CHECK(replace.Schedule(GateRequest(31, 500, 5, 10), 100).decision ==
          FrameGateDecision::Deferred);
    CHECK(replace.LiveCount() == 1);
    const FrameGateOutcome replaced = replace.Schedule(GateRequest(31, 501, 5, 20), 100);
    CHECK(replaced.decision == FrameGateDecision::Deferred);
    CHECK(replaced.replaced_existing);
    CHECK(replaced.slot == 0);
    CHECK(replace.LiveCount() == 1);
    CHECK(replace.Stats().replaced_same_id == 1);
    CHECK(replace.RecordAt(0).handle == 501);
    CHECK(replace.RecordAt(0).start_frame == 120);
    CHECK(replace.Schedule(GateRequest(32, 502, 5, 5), 100).slot == 1);
    // A same-id immediate request also replaces before it plays.
    CHECK(replace.Schedule(GateRequest(32, 503, 5, -1000), 100).replaced_existing);
    CHECK(replace.LiveCount() == 2);

    // The immediate path's dedupe hit does not remove the pending record: the
    // original's action on that record is unresolved, so the model only counts
    // it. It still retires on its own frame.
    CHECK(dedupe.RetireDue(110, ready, 4) == 1);
    CHECK(ready[0].sound_id == 21);
}

void TestFrameGateOverflowAndGeneration() {
    using namespace nosfix::audio;
    FrameGateSchedule gate;
    gate.SetEnabled(true);
    for (std::uint32_t index = 0; index < kFrameGateCapacity; ++index) {
        const FrameGateOutcome outcome = gate.Schedule(
            GateRequest(static_cast<std::int32_t>(index), 1000 + index, 5, 10), 500);
        CHECK(outcome.decision == FrameGateDecision::Deferred);
        CHECK(outcome.slot == index);
    }
    CHECK(gate.LiveCount() == kFrameGateCapacity);
    CHECK(gate.Stats().high_watermark == kFrameGateCapacity);

    // A full array drops a deferred request: the original returns without
    // storing it.
    const FrameGateOutcome full = gate.Schedule(GateRequest(500, 5000, 5, 10), 500);
    CHECK(full.decision == FrameGateDecision::TableFull);
    CHECK(full.slot == kFrameGateInvalidSlot);
    CHECK(gate.Stats().table_full == 1);
    CHECK(gate.LiveCount() == kFrameGateCapacity);
    // An immediate request has already started, so only its marker is lost.
    const FrameGateOutcome played = gate.Schedule(GateRequest(501, 5001, 5, -1000), 500);
    CHECK(played.decision == FrameGateDecision::ImmediateUnrecorded);
    CHECK(gate.Stats().immediate == 1);
    CHECK(gate.Stats().table_full == 2);

    // A generation change drops every stored cue without retiring it.
    gate.SetGeneration(1);
    CHECK(gate.LiveCount() == 0);
    CHECK(gate.Stats().abandoned_generation == kFrameGateCapacity);
    CHECK(gate.Stats().generation_changes == 1);
    CHECK(gate.RecordAt(0).sound_id == -1);
    FrameGateReady ready[4]{};
    CHECK(gate.RetireDue(100000, ready, 4) == 0);
    gate.SetGeneration(1); // no-op
    CHECK(gate.Stats().generation_changes == 1);
    CHECK(gate.Schedule(GateRequest(1, 1, 5, 0), 10).decision ==
          FrameGateDecision::Deferred);
    CHECK(gate.Generation() == 1);

    // -1 is the free marker and 0 is the cleared handle: both are refused.
    CHECK(gate.Schedule(GateRequest(-1, 1, 5, 10), 10).decision ==
          FrameGateDecision::Invalid);
    CHECK(gate.Schedule(GateRequest(2, 0, 5, 10), 10).decision ==
          FrameGateDecision::Invalid);
    CHECK(gate.Stats().invalid == 2);

    // The 32-bit signed add wraps like the original: 0xFFFFFFFE + 5 == 3.
    CHECK(gate.Schedule(GateRequest(3, 3, 5, 5), 0xFFFFFFFEu).decision ==
          FrameGateDecision::Deferred);
    CHECK(gate.RecordAt(1).start_frame == 3);

    // A too-small out array leaves the rest stored and counts them.
    FrameGateSchedule small;
    small.SetEnabled(true);
    CHECK(small.Schedule(GateRequest(41, 41, 5, 0), 10).decision ==
          FrameGateDecision::Deferred);
    CHECK(small.Schedule(GateRequest(42, 42, 5, 0), 10).decision ==
          FrameGateDecision::Deferred);
    CHECK(small.LiveCount() == 2);
    FrameGateReady one[1]{};
    CHECK(small.RetireDue(10, one, 1) == 1);
    CHECK(small.Stats().retire_unreported == 1);
    CHECK(small.LiveCount() == 1);
    CHECK(small.RetireDue(10, one, 1) == 1);
    CHECK(small.LiveCount() == 0);
    CHECK(small.RetireDue(10, nullptr, 4) == 0);
    CHECK(small.RetireDue(10, one, 0) == 0);
}

} // namespace

int main() {
    using nosfix::audio::KeysoundQueueStats;

    KeysoundQueue invalid_queue(100);
    CHECK(!invalid_queue.Valid());
    KeysoundQueue tiny_queue(32);
    CHECK(!tiny_queue.Valid());

    KeysoundQueue queue(64);
    CHECK(queue.Valid());
    CHECK(queue.Capacity() == 64);

    Mapper mapper;
    queue.SetMapper(&MapQpc, &mapper);

    // Invalid requests never reach the ring.
    KeysoundRequest bad;
    CHECK(queue.Push(bad).status == KeysoundPushStatus::InvalidArgument);
    bad = Request(0, 1, kQpcOrigin);
    CHECK(queue.Push(bad).status == KeysoundPushStatus::InvalidArgument);
    bad = Request(1, 0, kQpcOrigin);
    CHECK(queue.Push(bad).status == KeysoundPushStatus::InvalidArgument);
    bad = Request(1, 1, 0);
    CHECK(queue.Push(bad).status == KeysoundPushStatus::InvalidArgument);
    bad = Request(1, 1, kQpcOrigin);
    bad.pan = 2.0F;
    CHECK(queue.Push(bad).status == KeysoundPushStatus::InvalidArgument);
    bad = Request(1, 1, kQpcOrigin);
    bad.volume_db = std::numeric_limits<float>::quiet_NaN();
    CHECK(queue.Push(bad).status == KeysoundPushStatus::InvalidArgument);
    CHECK(queue.SnapshotStats().rejected_invalid == 6);

    // Once-only producer sequence, including a duplicate that carries new data.
    CHECK(queue.Push(Request(1, 7, kQpcOrigin + 1000, 10)).accepted());
    CHECK(queue.Push(Request(2, 7, kQpcOrigin + 2000, 11)).accepted());
    CHECK(queue.Push(Request(2, 7, kQpcOrigin + 2000, 12)).status ==
          KeysoundPushStatus::Duplicate);
    CHECK(queue.Push(Request(1, 7, kQpcOrigin + 1000, 12)).status ==
          KeysoundPushStatus::Duplicate);
    CHECK(queue.SnapshotStats().rejected_duplicate == 2);
    CHECK(queue.Pending() == 2);

    Sink sink;
    // Samples: request 1 = 4, request 2 = 8 (at 44.1 kHz, 1000 ticks ~= 4.41).
    KeysoundDispatchOutcome outcome =
        queue.DispatchDue(0, 128, 7, &Receive, &sink);
    CHECK(outcome.dispatched == 2);
    CHECK(sink.count == 2);
    CHECK(sink.items[0].cue_id == 10);
    CHECK(sink.items[0].offset_in_block == 4);
    CHECK(sink.items[0].late_frames == 0);
    CHECK(sink.items[1].cue_id == 11);
    CHECK(sink.items[1].offset_in_block == 9);
    CHECK(queue.Pending() == 0);

    // Second dispatch of the same block must not repeat anything.
    CHECK(queue.DispatchDue(0, 128, 7, &Receive, &sink).dispatched == 0);
    CHECK(sink.count == 2);

    // A future due time stays queued until the containing block.
    CHECK(queue.Push(Request(3, 7, kQpcOrigin + 10'000, 20)).accepted()); // 44
    CHECK(queue.DispatchDue(0, 32, 7, &Receive, &sink).dispatched == 0);
    CHECK(queue.Pending() == 1);
    CHECK(queue.DispatchDue(32, 32, 7, &Receive, &sink).dispatched == 1);
    CHECK(sink.items[2].offset_in_block == 12); // sample 44

    // Late event: due sample before the block start -> offset 0 and accounting.
    CHECK(queue.Push(Request(4, 7, kQpcOrigin - 100'000, 30)).accepted()); // -441
    outcome = queue.DispatchDue(0, 128, 7, &Receive, &sink);
    CHECK(outcome.dispatched == 1);
    CHECK(outcome.late == 1);
    CHECK(sink.items[3].offset_in_block == 0);
    CHECK(sink.items[3].late_frames == 441);
    CHECK(queue.SnapshotStats().total_late_frames == 441);
    CHECK(queue.SnapshotStats().max_late_frames == 441);

    // Out-of-order scheduled events: a future event at the head must not block
    // a later queued event whose due time is already inside this block.
    CHECK(queue.Push(Request(5, 7, kQpcOrigin + 60'000, 40)).accepted()); // 265
    CHECK(queue.Push(Request(6, 7, kQpcOrigin + 30'000, 41)).accepted()); // 132
    outcome = queue.DispatchDue(128, 128, 7, &Receive, &sink);
    CHECK(outcome.dispatched == 1);
    CHECK(sink.items[4].cue_id == 41);
    CHECK(sink.items[4].offset_in_block == 4);
    CHECK(queue.Pending() == 1);
    outcome = queue.DispatchDue(256, 128, 7, &Receive, &sink);
    CHECK(outcome.dispatched == 1);
    CHECK(sink.items[5].cue_id == 40);
    CHECK(sink.items[5].offset_in_block == 9);

    // Scheduled due_qpc is used instead of capture_qpc.
    KeysoundRequest scheduled = Request(7, 7, kQpcOrigin + 2000, 50);
    scheduled.due_qpc = kQpcOrigin + 50'000; // 220
    CHECK(queue.Push(scheduled).accepted());
    CHECK(queue.DispatchDue(0, 128, 7, &Receive, &sink).dispatched == 0);
    CHECK(queue.DispatchDue(128, 128, 7, &Receive, &sink).dispatched == 1);
    CHECK(sink.items[6].due_device_sample == 221);

    // Events from another generation are dropped, not played late.
    CHECK(queue.Push(Request(8, 7, kQpcOrigin + 1000, 60)).accepted());
    outcome = queue.DispatchDue(0, 128, 8, &Receive, &sink);
    CHECK(outcome.dispatched == 0);
    CHECK(outcome.dropped_stale == 1);
    CHECK(queue.Pending() == 0);
    CHECK(queue.SnapshotStats().dropped_stale_generation == 1);

    // Mapper failure leaves events queued; installing a working mapper
    // dispatches them exactly once.
    mapper.valid = false;
    CHECK(queue.Push(Request(9, 7, kQpcOrigin + 1000, 70)).accepted());
    CHECK(queue.DispatchDue(0, 128, 7, &Receive, &sink).dispatched == 0);
    CHECK(queue.Pending() == 1);
    mapper.valid = true;
    CHECK(queue.DispatchDue(0, 128, 7, &Receive, &sink).dispatched == 1);
    CHECK(sink.items[7].cue_id == 70);

    // Sink refusal is counted and the event is retired (never replayed).
    sink.reject = true;
    CHECK(queue.Push(Request(10, 7, kQpcOrigin + 1000, 80)).accepted());
    outcome = queue.DispatchDue(0, 128, 7, &Receive, &sink);
    CHECK(outcome.dispatched == 1);
    CHECK(outcome.sink_rejected == 1);
    CHECK(sink.rejected == 1);
    CHECK(queue.Pending() == 0);
    sink.reject = false;
    CHECK(queue.DispatchDue(0, 128, 7, &Receive, &sink).dispatched == 0);

    // Overflow is explicit and counted; the newest request is rejected.
    KeysoundQueue small(64);
    small.SetMapper(&MapQpc, &mapper);
    for (std::uint64_t i = 1; i <= 64; ++i) {
        CHECK(small.Push(Request(i, 9, kQpcOrigin + static_cast<std::int64_t>(i))).accepted());
    }
    CHECK(small.Push(Request(65, 9, kQpcOrigin + 65)).status ==
          KeysoundPushStatus::Overflow);
    KeysoundQueueStats stats = small.SnapshotStats();
    CHECK(stats.rejected_overflow == 1);
    CHECK(stats.pending == 64);
    CHECK(stats.high_watermark == 64);
    Sink overflow_sink;
    outcome = small.DispatchDue(0, 128, 9, &Receive, &overflow_sink);
    CHECK(outcome.dispatched == 64);
    CHECK(overflow_sink.count == 64);
    CHECK(small.Pending() == 0);
    CHECK(small.SnapshotStats().dispatched == 64);

    // Allocation freedom for push and dispatch, including the late path.
    KeysoundQueue rt_queue(64);
    rt_queue.SetMapper(&MapQpc, &mapper);
    for (std::uint64_t i = 1; i <= 32; ++i) {
        CHECK(rt_queue.Push(Request(100 + i, 11, kQpcOrigin +
                                    static_cast<std::int64_t>(i * 50))).accepted());
    }
    Sink rt_sink;
    test::BeginAllocationProbe();
    const KeysoundDispatchOutcome rt_outcome =
        rt_queue.DispatchDue(0, 128, 11, &Receive, &rt_sink);
    const long long allocations = test::EndAllocationProbe();
    CHECK(rt_outcome.dispatched == 32);
    CHECK(allocations == 0);

    // Pending is an undispatched-event count; an event remains pending until a
    // block containing its due time is dispatched.
    CHECK(rt_queue.Push(Request(200, 11, kQpcOrigin + 1000)).accepted());
    CHECK(rt_queue.Pending() == 1);
    CHECK(rt_queue.DispatchDue(0, 128, 11, &Receive, &rt_sink).dispatched == 1);
    CHECK(rt_queue.Pending() == 0);

    // Concurrent producer/consumer stress: correct SPSC publication must not
    // lose, duplicate or transiently underflow the pending count.
    {
        KeysoundQueue concurrent(1024);
        concurrent.SetMapper(&StressMap, nullptr);
        StressSink stress_sink;
        for (auto& slot : stress_sink.seen) slot.store(0, std::memory_order_relaxed);
        std::atomic<std::uint32_t> accepted{0};
        std::atomic<bool> producer_done{false};
        std::thread producer([&]() {
            for (std::uint32_t sequence = 1; sequence <= kStressEvents; ++sequence) {
                for (;;) {
                    const auto result = concurrent.Push(
                        Request(sequence, 1, kQpcOrigin + sequence));
                    if (result.accepted()) {
                        accepted.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }
                    if (result.status != KeysoundPushStatus::Overflow) break;
                    std::this_thread::yield();
                }
            }
            producer_done.store(true, std::memory_order_release);
        });
        std::thread consumer([&]() {
            std::int64_t block_start = 0;
            for (int iteration = 0; iteration < 200'000; ++iteration) {
                (void)concurrent.DispatchDue(block_start, 256, 1, &StressReceive,
                                             &stress_sink);
                block_start += 256;
                if (producer_done.load(std::memory_order_acquire) &&
                    concurrent.Pending() == 0) {
                    break;
                }
                std::this_thread::yield();
            }
        });
        producer.join();
        consumer.join();
        const KeysoundQueueStats stress_stats = concurrent.SnapshotStats();
        CHECK(accepted.load() == kStressEvents);
        CHECK(stress_sink.count.load() == kStressEvents);
        CHECK(stress_sink.duplicates.load() == 0);
        CHECK(stress_stats.pushed == kStressEvents);
        CHECK(stress_stats.dispatched == kStressEvents);
        CHECK(stress_stats.pending == 0);
        CHECK(stress_stats.pending <= concurrent.Capacity());
        CHECK(stress_stats.high_watermark <= concurrent.Capacity());
    }

    // Extreme mapper output: a late INT64_MIN due sample must not overflow the
    // signed late-frame subtraction; the reported lateness saturates.
    {
        KeysoundQueue edge(64);
        edge.SetMapper(&MinimumMap, nullptr);
        Sink edge_sink;
        CHECK(edge.Push(Request(1, 5, 1)).accepted());
        const auto edge_outcome = edge.DispatchDue(1'000'000, 128, 5,
                                                   &Receive, &edge_sink);
        CHECK(edge_outcome.dispatched == 1);
        CHECK(edge_outcome.late == 1);
        CHECK(edge_sink.count == 1);
        CHECK(edge_sink.items[0].offset_in_block == 0);
        CHECK(edge_sink.items[0].late_frames ==
              std::numeric_limits<std::uint32_t>::max());
    }


    // --- mixer integration -------------------------------------------------
    // The queue guarantees one delivery with a block offset; these tests drive
    // the real AudioMixer sink so the offset is proven on rendered samples, not
    // just in the dispatch record.
    {
        using nosfix::audio::AudioMixer;
        using nosfix::audio::AudioEncoding;
        using nosfix::audio::MixerBufferDesc;
        using nosfix::audio::MixerFormat;
        using nosfix::audio::MixerVoiceDesc;

        struct MixerSink {
            AudioMixer* mixer = nullptr;
            std::int32_t voice = 0;
            std::uint32_t starts = 0;
            std::uint32_t rejected = 0;
            std::uint32_t last_offset = 0;
        };
        auto start_in_mixer = [](void* context,
                                 const KeysoundDispatch& dispatch) noexcept -> bool {
            auto* sink = static_cast<MixerSink*>(context);
            sink->last_offset = dispatch.offset_in_block;
            if (sink->voice <= 0 || !sink->mixer->VoiceAlive(sink->voice)) {
                ++sink->rejected;
                return false;
            }
            if (!sink->mixer->StartVoice(sink->voice, dispatch.offset_in_block)) {
                ++sink->rejected;
                return false;
            }
            ++sink->starts;
            return true;
        };

        static AudioMixer mixer;
        CHECK(mixer.Prepare(44100, 2, 256));
        MixerVoiceDesc desc;
        desc.format.sample_rate = 44100;
        desc.format.channels = 1;
        desc.format.encoding = AudioEncoding::Float32;
        const std::int32_t voice = mixer.CreateVoice(desc);
        CHECK(voice > 0);
        static const float one_sample[1] = {0.5F};
        MixerBufferDesc buffer;
        buffer.data = reinterpret_cast<const std::uint8_t*>(one_sample);
        buffer.frame_count = 1;
        buffer.loop_count = nosfix::audio::kMixerLoopInfinite;
        CHECK(mixer.SubmitBuffer(voice, buffer));

        MixerSink mixer_sink;
        mixer_sink.mixer = &mixer;
        mixer_sink.voice = voice;

        static Mapper mix_mapper;
        mix_mapper.valid = true;
        KeysoundQueue mix_queue(64);
        mix_queue.SetMapper(&MapQpc, &mix_mapper);
        // Sample 1037: origin 5000000 ticks -> 0 samples, so 1037 samples is
        // 1037 * 10'000'000 / 44100 ticks after the origin.
        const std::int64_t due_qpc = kQpcOrigin + (1037LL * kQpcFrequency) / kRate;
        CHECK(mix_queue.Push(Request(1, 9, due_qpc)).accepted());
        const auto mix_outcome = mix_queue.DispatchDue(1000, 128, 9, start_in_mixer, &mixer_sink);
        CHECK(mix_outcome.dispatched == 1);
        CHECK(mix_outcome.sink_rejected == 0);
        CHECK(mixer_sink.starts == 1);
        CHECK(mixer_sink.last_offset == 37);

        float left[128] = {};
        float right[128] = {};
        float* channels[2] = {left, right};
        CHECK(mixer.Render(1000, 128, channels, 2));
        for (int frame = 0; frame < 37; ++frame) {
            CHECK(left[frame] == 0.0F);
        }
        CHECK(std::fabs(left[37] - 0.5F) < 1e-6F);
        CHECK(std::fabs(right[127] - 0.5F) < 1e-6F);

        // A second dispatch of the same block delivers nothing: the event was
        // retired when the sink saw it.
        const auto repeat = mix_queue.DispatchDue(1000, 128, 9, start_in_mixer, &mixer_sink);
        CHECK(repeat.dispatched == 0);
        CHECK(mixer_sink.starts == 1);

        // Generation mismatch is dropped before the sink.
        CHECK(mix_queue.Push(Request(2, 10, due_qpc)).accepted());
        const auto stale = mix_queue.DispatchDue(1000, 128, 11, start_in_mixer, &mixer_sink);
        CHECK(stale.dispatched == 0);
        CHECK(stale.dropped_stale == 1);
        CHECK(mixer_sink.starts == 1);

        // A dead mixer handle is refused by the sink and counted once; the event
        // is still retired.
        CHECK(mix_queue.Push(Request(3, 9, due_qpc)).accepted());
        const std::int32_t dead = voice + 100000; // not a live handle
        MixerSink broken_mixer_sink;
        broken_mixer_sink.mixer = &mixer;
        broken_mixer_sink.voice = dead;
        const auto refused = mix_queue.DispatchDue(1000, 128, 9, start_in_mixer,
                                                   &broken_mixer_sink);
        CHECK(refused.dispatched == 1);
        CHECK(refused.sink_rejected == 1);
        CHECK(broken_mixer_sink.rejected == 1);
        CHECK(mix_queue.Pending() == 0);
    }

    TestFrameGateLayout();
    TestFrameGateDeferred();
    TestFrameGateImmediate();
    TestFrameGateOverflowAndGeneration();

    return test::Finish("keysound_dispatch");
}
