// Always-on tests for nosfix::InputNormalizer.
//
// These checks are deliberately NOT <cassert>: the parent build defines NDEBUG
// for Release, and a diagnostic-only check would vanish exactly where it
// matters. Every check below is evaluated, counted, and reported, and main()
// returns a nonzero exit code on any failure.
//
// Time is simulated with explicit QPC values. No sleeps, no threads, no OS
// calls: a consumer schedule is just a list of instants at which queued reports
// are drained.

#include "nosfix/input_normalizer.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <span>
#include <vector>

// Global replacement allocation hooks. The probe is off unless a test enables
// it, so the harness itself may still use std::vector; the realtime-path test
// turns it on around one Apply/Reset call and requires zero allocations.
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
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);      \
        }                                                                         \
    } while (false)

using nosfix::InputApplyStatus;
using nosfix::InputBinding;
using nosfix::InputConfigStatus;
using nosfix::InputEdge;
using nosfix::InputNormalizer;
using nosfix::InputResetStatus;
using nosfix::InputStamp;
using nosfix::LogicalInputEvent;
using nosfix::PhysicalControlState;

constexpr std::int64_t kQpcFrequency = 10'000'000;
constexpr std::int64_t kQpcMs = 10'000;                    // one millisecond
constexpr std::int64_t kQpcBase = 5'000'000'000'000LL;     // large uptime
constexpr std::size_t kCapacity = nosfix::kInputLaneCount; // 28 slots

// ---------------------------------------------------------------------------
// Builders and comparisons
// ---------------------------------------------------------------------------

InputStamp Stamp(std::int64_t qpc, std::uint64_t sequence, std::uint32_t ordinal = 0,
                 std::uint64_t epoch = 1) {
    InputStamp stamp;
    stamp.qpc = qpc;
    stamp.sequence = sequence;
    stamp.stream_epoch = epoch;
    stamp.ordinal = ordinal;
    return stamp;
}

PhysicalControlState Control(std::uint64_t source, std::uint64_t generation,
                             std::uint32_t control, bool down, float velocity) {
    PhysicalControlState state;
    state.source_id = source;
    state.device_generation = generation;
    state.control_id = control;
    state.down = down;
    state.velocity = velocity;
    return state;
}

InputBinding Bind(std::uint64_t source, std::uint64_t generation,
                  std::uint32_t control, std::uint8_t lane) {
    InputBinding binding;
    binding.source_id = source;
    binding.device_generation = generation;
    binding.control_id = control;
    binding.lane = lane;
    return binding;
}

bool SameStamp(const InputStamp& a, const InputStamp& b) {
    return a.qpc == b.qpc && a.sequence == b.sequence &&
           a.stream_epoch == b.stream_epoch && a.ordinal == b.ordinal;
}

bool SameEvent(const LogicalInputEvent& a, const LogicalInputEvent& b) {
    return SameStamp(a.stamp, b.stamp) && a.source_id == b.source_id &&
           a.device_generation == b.device_generation && a.lane == b.lane &&
           a.edge == b.edge && a.velocity == b.velocity &&
           a.synthetic == b.synthetic;
}

const char* EdgeName(InputEdge edge) {
    return edge == InputEdge::Press ? "Press" : "Release";
}

void ExpectEvent(const LogicalInputEvent& actual, const InputStamp& stamp,
                 std::uint64_t source_id, std::uint64_t device_generation,
                 std::uint8_t lane, InputEdge edge, float velocity, bool synthetic,
                 const char* label, const char* file, int line) {
    ++g_checks;
    const bool ok = SameStamp(actual.stamp, stamp) && actual.source_id == source_id &&
                    actual.device_generation == device_generation &&
                    actual.lane == lane && actual.edge == edge &&
                    actual.velocity == velocity && actual.synthetic == synthetic;
    if (ok) return;
    ++g_failures;
    std::printf(
        "FAIL %s:%d %s\n"
        "  expected qpc=%lld seq=%llu epoch=%llu ord=%u src=%llu gen=%llu lane=%u edge=%s vel=%.3f synthetic=%d\n"
        "  actual   qpc=%lld seq=%llu epoch=%llu ord=%u src=%llu gen=%llu lane=%u edge=%s vel=%.3f synthetic=%d\n",
        file, line, label,
        static_cast<long long>(stamp.qpc),
        static_cast<unsigned long long>(stamp.sequence),
        static_cast<unsigned long long>(stamp.stream_epoch),
        static_cast<unsigned>(stamp.ordinal),
        static_cast<unsigned long long>(source_id),
        static_cast<unsigned long long>(device_generation),
        static_cast<unsigned>(lane), EdgeName(edge),
        static_cast<double>(velocity), synthetic ? 1 : 0,
        static_cast<long long>(actual.stamp.qpc),
        static_cast<unsigned long long>(actual.stamp.sequence),
        static_cast<unsigned long long>(actual.stamp.stream_epoch),
        static_cast<unsigned>(actual.stamp.ordinal),
        static_cast<unsigned long long>(actual.source_id),
        static_cast<unsigned long long>(actual.device_generation),
        static_cast<unsigned>(actual.lane), EdgeName(actual.edge),
        static_cast<double>(actual.velocity), actual.synthetic ? 1 : 0);
}

#define EXPECT_EVENT(event, stamp, source, generation, lane, edge, velocity, synthetic) \
    ExpectEvent((event), (stamp), (source), (generation), (lane), (edge), (velocity),    \
                (synthetic), #event, __FILE__, __LINE__)

// Applies one report through a buffer of `capacity` slots and appends whatever
// was written. The rest of the buffer is checked separately where it matters.
InputApplyStatus ApplyReport(InputNormalizer& normalizer,
                             const PhysicalControlState& state,
                             const InputStamp& stamp,
                             std::vector<LogicalInputEvent>& events,
                             std::size_t capacity = kCapacity,
                             std::size_t* required = nullptr) {
    std::array<LogicalInputEvent, kCapacity> buffer{};
    const auto result = normalizer.Apply(
        state, stamp, std::span<LogicalInputEvent>(buffer.data(), capacity));
    if (required != nullptr) *required = result.required_event_count;
    for (std::size_t i = 0; i < result.event_count; ++i) {
        events.push_back(buffer[i]);
    }
    return result.status;
}

constexpr std::uint64_t kSentinel = 0x5A5A5A5A5A5A5A5AULL;

template <typename Callable>
std::size_t AllocationsDuring(Callable&& callable) {
    allocation_probe::count = 0;
    allocation_probe::tracking = true;
    callable();
    allocation_probe::tracking = false;
    return static_cast<std::size_t>(allocation_probe::count);
}

void FillSentinel(std::span<LogicalInputEvent> buffer) {
    for (LogicalInputEvent& event : buffer) {
        event.stamp = Stamp(-7, kSentinel, 0xDEADBEEFu, kSentinel);
        event.source_id = kSentinel;
        event.device_generation = kSentinel;
        event.lane = 0xFF;
        event.edge = InputEdge::Press;
        event.velocity = -1.0f;
        event.synthetic = true;
    }
}

bool SentinelIntact(std::span<const LogicalInputEvent> buffer) {
    for (const LogicalInputEvent& event : buffer) {
        if (event.stamp.qpc != -7 || event.stamp.sequence != kSentinel ||
            event.stamp.stream_epoch != kSentinel ||
            event.stamp.ordinal != 0xDEADBEEFu || event.source_id != kSentinel ||
            event.device_generation != kSentinel || event.lane != 0xFF ||
            event.edge != InputEdge::Press || event.velocity != -1.0f ||
            !event.synthetic) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 1. Configuration: bounds, validation, transactionality, held rebinds
// ---------------------------------------------------------------------------

void TestConfigurationValidation() {
    std::printf("configuration validation\n");
    InputNormalizer normalizer;
    NOSFIX_CHECK(normalizer.BindingCount() == 0);
    NOSFIX_CHECK(!normalizer.HasAdoptedEpoch());
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(normalizer.HeldControlCount() == 0);

    // The documented bound is usable; one more binding is rejected whole.
    std::vector<InputBinding> full;
    full.reserve(nosfix::kInputMaxBindings + 1);
    for (std::size_t i = 0; i < nosfix::kInputMaxBindings + 1; ++i) {
        full.push_back(Bind(1, 1, static_cast<std::uint32_t>(i + 1),
                            static_cast<std::uint8_t>(i % nosfix::kInputLaneCount)));
    }
    NOSFIX_CHECK(normalizer.Configure(
                     std::span<const InputBinding>(full.data(), nosfix::kInputMaxBindings + 1)) ==
                 InputConfigStatus::RejectedTooManyBindings);
    NOSFIX_CHECK(normalizer.BindingCount() == 0);
    NOSFIX_CHECK(normalizer.Configure(
                     std::span<const InputBinding>(full.data(), nosfix::kInputMaxBindings)) ==
                 InputConfigStatus::Accepted);
    NOSFIX_CHECK(normalizer.BindingCount() == nosfix::kInputMaxBindings);

    // Control 256 is the last binding, on lane 255 % 28 == 3.
    std::vector<LogicalInputEvent> events;
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 256, true, 0.5f),
                             Stamp(kQpcBase, 1), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 1), 1, 1, 3, InputEdge::Press, 0.5f, false);
    }
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 256, false, 0.0f),
                             Stamp(kQpcBase + kQpcMs, 2), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);

    // Invalid fields are rejected without installing anything.
    const std::array<InputBinding, 1> bad_lane{Bind(1, 1, 5, 28)};
    const std::array<InputBinding, 1> zero_source{Bind(0, 1, 5, 0)};
    const std::array<InputBinding, 1> zero_generation{Bind(1, 0, 5, 0)};
    const std::array<InputBinding, 1> zero_control{Bind(1, 1, 0, 0)};
    const std::array<InputBinding, 2> duplicate{Bind(1, 1, 5, 4), Bind(1, 1, 5, 4)};

    InputNormalizer fresh;
    NOSFIX_CHECK(fresh.Configure(bad_lane) == InputConfigStatus::RejectedInvalidBinding);
    NOSFIX_CHECK(fresh.Configure(zero_source) == InputConfigStatus::RejectedInvalidBinding);
    NOSFIX_CHECK(fresh.Configure(zero_generation) == InputConfigStatus::RejectedInvalidBinding);
    NOSFIX_CHECK(fresh.Configure(zero_control) == InputConfigStatus::RejectedInvalidBinding);
    NOSFIX_CHECK(fresh.Configure(duplicate) == InputConfigStatus::RejectedDuplicateBinding);
    NOSFIX_CHECK(fresh.BindingCount() == 0);

    // One control may bind several lanes; that is not a duplicate.
    const std::array<InputBinding, 2> multi_lane{Bind(1, 1, 5, 4), Bind(1, 1, 5, 9)};
    NOSFIX_CHECK(fresh.Configure(multi_lane) == InputConfigStatus::Accepted);
    NOSFIX_CHECK(fresh.BindingCount() == 2);

    // A rejected configuration leaves the previous table fully in service.
    InputNormalizer transactional;
    const std::array<InputBinding, 2> a_on_zero{Bind(1, 1, 10, 0), Bind(1, 1, 11, 0)};
    NOSFIX_CHECK(transactional.Configure(a_on_zero) == InputConfigStatus::Accepted);
    const std::array<InputBinding, 2> partly_invalid{Bind(9, 9, 9, 0), Bind(0, 1, 1, 0)};
    NOSFIX_CHECK(transactional.Configure(partly_invalid) ==
                 InputConfigStatus::RejectedInvalidBinding);
    NOSFIX_CHECK(transactional.BindingCount() == 2);
    events.clear();
    NOSFIX_CHECK(ApplyReport(transactional, Control(1, 1, 10, true, 0.5f),
                             Stamp(kQpcBase, 1), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 1), 1, 1, 0, InputEdge::Press, 0.5f, false);
    }
    // The rejected table's control 9 never became mapped.
    events.clear();
    NOSFIX_CHECK(ApplyReport(transactional, Control(9, 9, 9, true, 0.5f),
                             Stamp(kQpcBase + kQpcMs, 2), events) ==
                 InputApplyStatus::IgnoredUnmappedControl);
    NOSFIX_CHECK(events.empty());

    // Empty configuration is a rebind of the held control: refused until the
    // lane has been released.
    NOSFIX_CHECK(transactional.Configure(std::span<const InputBinding>()) ==
                 InputConfigStatus::RejectedHeldControlRebind);
    NOSFIX_CHECK(transactional.BindingCount() == 2);
    events.clear();
    NOSFIX_CHECK(ApplyReport(transactional, Control(1, 1, 10, false, 0.0f),
                             Stamp(kQpcBase + 2 * kQpcMs, 3), events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    NOSFIX_CHECK(transactional.Configure(std::span<const InputBinding>()) ==
                 InputConfigStatus::Accepted);
    NOSFIX_CHECK(transactional.BindingCount() == 0);
}

void TestHeldControlRebindIsRejected() {
    std::printf("held-control rebind rejection\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 3> initial{
        Bind(1, 1, 10, 0), Bind(1, 1, 11, 0), Bind(2, 3, 7, 4)};
    NOSFIX_CHECK(normalizer.Configure(initial) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f),
                             Stamp(kQpcBase, 1), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(normalizer.IsControlHeld(1, 1, 10));
    events.clear();

    // Moving the held control to another lane, dropping it, or changing its
    // lane set is a rebind and must wait for a Reset.
    const std::array<InputBinding, 3> moved{Bind(1, 1, 10, 1), Bind(1, 1, 11, 0),
                                            Bind(2, 3, 7, 4)};
    const std::array<InputBinding, 2> dropped{Bind(1, 1, 11, 0), Bind(2, 3, 7, 4)};
    const std::array<InputBinding, 4> widened{Bind(1, 1, 10, 0), Bind(1, 1, 10, 2),
                                              Bind(1, 1, 11, 0), Bind(2, 3, 7, 4)};
    NOSFIX_CHECK(normalizer.Configure(moved) == InputConfigStatus::RejectedHeldControlRebind);
    NOSFIX_CHECK(normalizer.Configure(dropped) == InputConfigStatus::RejectedHeldControlRebind);
    NOSFIX_CHECK(normalizer.Configure(widened) == InputConfigStatus::RejectedHeldControlRebind);
    NOSFIX_CHECK(normalizer.BindingCount() == 3);

    // Adding a binding for a control that is not held is fine.
    const std::array<InputBinding, 4> added{Bind(1, 1, 10, 0), Bind(1, 1, 11, 0),
                                            Bind(2, 3, 7, 4), Bind(1, 1, 12, 7)};
    NOSFIX_CHECK(normalizer.Configure(added) == InputConfigStatus::Accepted);
    NOSFIX_CHECK(normalizer.BindingCount() == 4);

    // The held lane survived the accepted reconfiguration untouched.
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x1u);

    // Reset releases the held lane; only then is the rebind acceptable.
    std::array<LogicalInputEvent, kCapacity> buffer{};
    auto reset = normalizer.Reset(Stamp(kQpcBase + kQpcMs, 2, 0, 2), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 1);
    EXPECT_EVENT(buffer[0], Stamp(kQpcBase + kQpcMs, 2, 0, 2), 1, 1, 0,
                 InputEdge::Release, 0.0f, true);
    NOSFIX_CHECK(normalizer.HeldControlCount() == 0);
    NOSFIX_CHECK(normalizer.Configure(moved) == InputConfigStatus::Accepted);
    NOSFIX_CHECK(normalizer.BindingCount() == 3);
}

// ---------------------------------------------------------------------------
// 2. Press/release preservation, redundant levels, velocity while held
// ---------------------------------------------------------------------------

void TestEdgesAndRedundancy() {
    std::printf("edges, typematic, redundant levels\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 1> table{Bind(1, 1, 10, 0)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    const auto press_stamp = Stamp(kQpcBase, 100);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.75f), press_stamp,
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], press_stamp, 1, 1, 0, InputEdge::Press, 0.75f, false);
    }
    NOSFIX_CHECK(normalizer.HasAdoptedEpoch());
    NOSFIX_CHECK(normalizer.Epoch() == 1);
    NOSFIX_CHECK(SameStamp(normalizer.ConsumedStamp(), press_stamp));
    NOSFIX_CHECK(normalizer.IsLaneHeld(0));
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x1u);

    // Keyboard typematic: the same make level repeated must not press again.
    events.clear();
    for (std::uint64_t i = 0; i < 60; ++i) {
        const auto stamp = Stamp(kQpcBase + static_cast<std::int64_t>(i + 1) * kQpcMs,
                                 101 + i);
        NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.75f), stamp,
                                 events) == InputApplyStatus::Applied);
    }
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(normalizer.IsControlHeld(1, 1, 10));
    NOSFIX_CHECK(normalizer.HeldControlCount() == 1);

    // Velocity changes while held update the tracked value but are not edges.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.10f),
                             Stamp(kQpcBase + 200 * kQpcMs, 200), events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(normalizer.IsLaneHeld(0));

    // A release always reports velocity zero, whatever the decoder sends.
    const auto release_stamp = Stamp(kQpcBase + 201 * kQpcMs, 201);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, false, 0.9f), release_stamp,
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], release_stamp, 1, 1, 0, InputEdge::Release, 0.0f, false);
    }
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(normalizer.HeldControlCount() == 0);

    // A redundant release is not an edge either.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, false, 0.0f),
                             Stamp(kQpcBase + 202 * kQpcMs, 202), events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.empty());

    // Re-pressing after release is a real new press.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.33f),
                             Stamp(kQpcBase + 203 * kQpcMs, 203), events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase + 203 * kQpcMs, 203), 1, 1, 0,
                     InputEdge::Press, 0.33f, false);
    }

    // A press and a release inside one 60 Hz interval are both preserved when
    // drained together; the caller's cadence never merges them.
    events.clear();
    InputNormalizer fast;
    NOSFIX_CHECK(fast.Configure(table) == InputConfigStatus::Accepted);
    NOSFIX_CHECK(ApplyReport(fast, Control(1, 1, 10, true, 0.6f),
                             Stamp(kQpcBase, 1), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(ApplyReport(fast, Control(1, 1, 10, false, 0.0f),
                             Stamp(kQpcBase + 1 * kQpcMs, 2), events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 2);
    if (events.size() == 2) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 1), 1, 1, 0, InputEdge::Press, 0.6f, false);
        EXPECT_EVENT(events[1], Stamp(kQpcBase + 1 * kQpcMs, 2), 1, 1, 0,
                     InputEdge::Release, 0.0f, false);
    }
}

// ---------------------------------------------------------------------------
// 3. Chords, multi-lane controls, lane ordering, OR aggregation
// ---------------------------------------------------------------------------

void TestChordsAndMultipleMappings() {
    std::printf("chords and multi-lane mappings\n");
    InputNormalizer normalizer;
    // Table order is deliberately not lane order: emissions must be sorted.
    const std::array<InputBinding, 4> table{Bind(2, 3, 7, 9), Bind(2, 3, 8, 27),
                                            Bind(2, 3, 7, 4), Bind(1, 1, 10, 0)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    const auto chord_stamp_c = Stamp(kQpcBase, 500, 0);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 7, true, 0.7f), chord_stamp_c,
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 2);
    if (events.size() == 2) {
        EXPECT_EVENT(events[0], chord_stamp_c, 2, 3, 4, InputEdge::Press, 0.7f, false);
        EXPECT_EVENT(events[1], chord_stamp_c, 2, 3, 9, InputEdge::Press, 0.7f, false);
    }

    // Second note of the chord: same QPC and sequence, next ordinal.
    events.clear();
    const auto chord_stamp_d = Stamp(kQpcBase, 500, 1);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 8, true, 0.9f), chord_stamp_d,
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], chord_stamp_d, 2, 3, 27, InputEdge::Press, 0.9f, false);
    }
    NOSFIX_CHECK(normalizer.HeldLaneMask() == ((1u << 4) | (1u << 9) | (1u << 27)));

    // Releasing the chord keeps lane order and per-control identity.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 8, false, 0.0f),
                             Stamp(kQpcBase, 500, 2), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 500, 2), 2, 3, 27, InputEdge::Release, 0.0f,
                     false);
    }
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 7, false, 0.0f),
                             Stamp(kQpcBase, 500, 3), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 2);
    if (events.size() == 2) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 500, 3), 2, 3, 4, InputEdge::Release, 0.0f,
                     false);
        EXPECT_EVENT(events[1], Stamp(kQpcBase, 500, 3), 2, 3, 9, InputEdge::Release, 0.0f,
                     false);
    }
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
}

void TestSharedLaneAggregation() {
    std::printf("shared-lane OR aggregation\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 2> table{Bind(1, 1, 10, 0), Bind(1, 1, 11, 0)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    const auto a_press = Stamp(kQpcBase, 10);
    const auto b_press = Stamp(kQpcBase, 10, 1);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.6f), a_press, events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 11, true, 0.5f), b_press, events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.empty()); // lane already held
    NOSFIX_CHECK(normalizer.IsLaneHeld(0));

    // Releasing one of two held alternatives must not release the lane.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, false, 0.0f),
                             Stamp(kQpcBase, 10, 2), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(normalizer.IsLaneHeld(0));
    NOSFIX_CHECK(!normalizer.IsControlHeld(1, 1, 10));
    NOSFIX_CHECK(normalizer.IsControlHeld(1, 1, 11));

    // The last release does release the lane, with the triggering identity.
    const auto final_release = Stamp(kQpcBase, 10, 3);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 11, false, 0.0f), final_release,
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], final_release, 1, 1, 0, InputEdge::Release, 0.0f, false);
    }
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);

    // The reverse order behaves the same way.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 11, true, 0.5f),
                             Stamp(kQpcBase, 11, 0), events) == InputApplyStatus::Applied);
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.6f),
                             Stamp(kQpcBase, 11, 1), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.empty());
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 11, false, 0.0f),
                             Stamp(kQpcBase, 11, 2), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.empty());
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, false, 0.0f),
                             Stamp(kQpcBase, 11, 3), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 11, 3), 1, 1, 0, InputEdge::Release, 0.0f,
                     false);
    }
}

// ---------------------------------------------------------------------------
// 4. Unmapped controls, stamp validation, epochs
// ---------------------------------------------------------------------------

void TestUnmappedControl() {
    std::printf("unmapped control is ignored, not fabricated\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 1> table{Bind(1, 1, 10, 0)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    const auto stamp = Stamp(kQpcBase, 1);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 99, true, 0.5f), stamp, events) ==
                 InputApplyStatus::IgnoredUnmappedControl);
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(normalizer.HeldControlCount() == 0);

    // The ignored report is still consumed, so the same stamp cannot repeat.
    std::array<LogicalInputEvent, kCapacity> buffer{};
    const auto replay = normalizer.Apply(Control(1, 1, 10, true, 0.5f), stamp, buffer);
    NOSFIX_CHECK(replay.status == InputApplyStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(!replay.Consumed());
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);

    // A different unmapped control at the next stamp is ignored again.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 2, 10, true, 0.5f),
                             Stamp(kQpcBase, 2), events) ==
                 InputApplyStatus::IgnoredUnmappedControl); // wrong device generation
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f),
                             Stamp(kQpcBase, 3), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
}

void TestStampValidation() {
    std::printf("stamp validation and ordering\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 1> table{Bind(1, 1, 10, 0)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    // Zero epoch: the capture stream has not established identity.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f),
                             Stamp(kQpcBase, 1, 0, 0), events) ==
                 InputApplyStatus::RejectedInvalidStamp);
    // Zero sequence: no ordering possible.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f),
                             Stamp(kQpcBase, 0), events) ==
                 InputApplyStatus::RejectedInvalidStamp);
    // Negative QPC.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f),
                             Stamp(-1, 1), events) == InputApplyStatus::RejectedInvalidStamp);
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(!normalizer.HasAdoptedEpoch());
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);

    // Invalid report fields are rejected before the stamp is consumed.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(0, 1, 10, true, 0.5f), Stamp(kQpcBase, 1),
                             events) == InputApplyStatus::RejectedInvalidReport);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 0, 10, true, 0.5f), Stamp(kQpcBase, 1),
                             events) == InputApplyStatus::RejectedInvalidReport);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 0, true, 0.5f), Stamp(kQpcBase, 1),
                             events) == InputApplyStatus::RejectedInvalidReport);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true,
                                                 std::numeric_limits<float>::quiet_NaN()),
                             Stamp(kQpcBase, 1), events) ==
                 InputApplyStatus::RejectedInvalidReport);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true,
                                                 std::numeric_limits<float>::infinity()),
                             Stamp(kQpcBase, 1), events) ==
                 InputApplyStatus::RejectedInvalidReport);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 1.5f), Stamp(kQpcBase, 1),
                             events) == InputApplyStatus::RejectedInvalidReport);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, -0.5f), Stamp(kQpcBase, 1),
                             events) == InputApplyStatus::RejectedInvalidReport);
    NOSFIX_CHECK(events.empty());

    // A batch of reports sharing one QPC and sequence is valid; only the
    // ordinal separates them.
    const std::array<InputBinding, 2> more{Bind(1, 1, 10, 0), Bind(1, 1, 11, 0)};
    InputNormalizer batched;
    NOSFIX_CHECK(batched.Configure(more) == InputConfigStatus::Accepted);
    const std::int64_t batch_qpc = kQpcBase + 7 * kQpcMs;
    std::vector<LogicalInputEvent> batch_events;
    NOSFIX_CHECK(ApplyReport(batched, Control(1, 1, 10, true, 0.5f),
                             Stamp(batch_qpc, 42, 0), batch_events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(ApplyReport(batched, Control(1, 1, 11, true, 0.6f),
                             Stamp(batch_qpc, 42, 1), batch_events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(ApplyReport(batched, Control(1, 1, 10, false, 0.0f),
                             Stamp(batch_qpc, 42, 2), batch_events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(ApplyReport(batched, Control(1, 1, 11, false, 0.0f),
                             Stamp(batch_qpc, 42, 3), batch_events) ==
                 InputApplyStatus::Applied);
    // Lane 0 is shared, so 10's release does not release it; 11's does. Only
    // the first press and the final release are edges.
    NOSFIX_CHECK(batch_events.size() == 2);
    NOSFIX_CHECK(batched.HeldLaneMask() == 0);

    // Backwards QPC within the epoch is rejected and consumes nothing.
    InputNormalizer ordered;
    NOSFIX_CHECK(ordered.Configure(table) == InputConfigStatus::Accepted);
    std::vector<LogicalInputEvent> ordered_events;
    NOSFIX_CHECK(ApplyReport(ordered, Control(1, 1, 10, true, 0.5f),
                             Stamp(kQpcBase + 10 * kQpcMs, 10, 3), ordered_events) ==
                 InputApplyStatus::Applied);
    const auto before_bad = ordered.ConsumedStamp();
    const std::uint32_t before_mask = ordered.HeldLaneMask();
    std::array<LogicalInputEvent, kCapacity> buffer{};
    FillSentinel(buffer);
    auto result = ordered.Apply(Control(1, 1, 10, false, 0.0f),
                                Stamp(kQpcBase + 9 * kQpcMs, 11, 0), buffer);
    NOSFIX_CHECK(result.status == InputApplyStatus::RejectedBackwardStamp);
    NOSFIX_CHECK(!result.Consumed());
    NOSFIX_CHECK(SameStamp(ordered.ConsumedStamp(), before_bad));
    NOSFIX_CHECK(ordered.HeldLaneMask() == before_mask);
    NOSFIX_CHECK(SentinelIntact(buffer));

    // Backwards ordinal inside one sequence is rejected.
    result = ordered.Apply(Control(1, 1, 10, false, 0.0f), Stamp(kQpcBase + 11 * kQpcMs, 10, 2),
                           buffer);
    NOSFIX_CHECK(result.status == InputApplyStatus::RejectedBackwardStamp);
    NOSFIX_CHECK(ordered.HeldLaneMask() == before_mask);

    // An older sequence is rejected even with a later QPC.
    result = ordered.Apply(Control(1, 1, 10, false, 0.0f), Stamp(kQpcBase + 12 * kQpcMs, 9, 5),
                           buffer);
    NOSFIX_CHECK(result.status == InputApplyStatus::RejectedBackwardStamp);
    NOSFIX_CHECK(ordered.HeldLaneMask() == before_mask);

    // Duplicate stamp: the same report identity never replays an event, even
    // when its payload asks for the opposite edge. The identity, not the QPC,
    // is what makes a delivery a duplicate.
    result = ordered.Apply(Control(1, 1, 10, false, 0.0f), Stamp(kQpcBase + 10 * kQpcMs, 10, 3),
                           buffer);
    NOSFIX_CHECK(result.status == InputApplyStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(result.event_count == 0);
    NOSFIX_CHECK(ordered.HeldLaneMask() == before_mask);
    NOSFIX_CHECK(SentinelIntact(buffer));
    result = ordered.Apply(Control(1, 1, 10, false, 0.0f), Stamp(kQpcBase + 14 * kQpcMs, 10, 3),
                           buffer);
    NOSFIX_CHECK(result.status == InputApplyStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(ordered.HeldLaneMask() == before_mask);
    NOSFIX_CHECK(SameStamp(ordered.ConsumedStamp(), before_bad));

    // The lane is still held, and the legitimate release still works.
    NOSFIX_CHECK(ApplyReport(ordered, Control(1, 1, 10, false, 0.0f),
                             Stamp(kQpcBase + 13 * kQpcMs, 11, 0), ordered_events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(ordered_events.size() == 2);
    NOSFIX_CHECK(ordered.HeldLaneMask() == 0);
}

void TestEpochs() {
    std::printf("epoch adoption and boundaries\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 1> table{Bind(1, 1, 10, 0)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    // The first valid report adopts its epoch implicitly: nothing is held yet.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f),
                             Stamp(kQpcBase, 1, 0, 7), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(normalizer.Epoch() == 7);
    NOSFIX_CHECK(events.size() == 1);

    // A newer epoch without Reset is refused; state and cursor are untouched.
    const auto before = normalizer.ConsumedStamp();
    std::array<LogicalInputEvent, kCapacity> buffer{};
    FillSentinel(buffer);
    auto result = normalizer.Apply(Control(1, 1, 10, false, 0.0f),
                                   Stamp(kQpcBase + kQpcMs, 2, 0, 8), buffer);
    NOSFIX_CHECK(result.status == InputApplyStatus::RejectedEpochChangeRequiresReset);
    NOSFIX_CHECK(SameStamp(normalizer.ConsumedStamp(), before));
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x1u);
    NOSFIX_CHECK(SentinelIntact(buffer));

    // An older epoch is stale; a stale reset is refused too.
    result = normalizer.Apply(Control(1, 1, 10, false, 0.0f),
                              Stamp(kQpcBase + kQpcMs, 2, 0, 6), buffer);
    NOSFIX_CHECK(result.status == InputApplyStatus::RejectedStaleEpoch);
    auto reset = normalizer.Reset(Stamp(kQpcBase + kQpcMs, 2, 0, 6), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedStaleEpoch);
    NOSFIX_CHECK(normalizer.Epoch() == 7);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x1u);

    // Explicit reset crosses the boundary and releases held state.
    reset = normalizer.Reset(Stamp(kQpcBase + kQpcMs, 2, 0, 8), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 1);
    NOSFIX_CHECK(normalizer.Epoch() == 8);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(SameStamp(normalizer.ConsumedStamp(), Stamp(kQpcBase + kQpcMs, 2, 0, 8)));

    // Reports from the new epoch are accepted; the old epoch is now stale.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.4f),
                             Stamp(kQpcBase + 2 * kQpcMs, 3, 0, 8), events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase + 2 * kQpcMs, 3, 0, 8), 1, 1, 0,
                     InputEdge::Press, 0.4f, false);
    }

    // A late report from the abandoned epoch is rejected as stale.
    result = normalizer.Apply(Control(1, 1, 10, false, 0.0f),
                              Stamp(kQpcBase + 3 * kQpcMs, 4, 0, 7), buffer);
    NOSFIX_CHECK(result.status == InputApplyStatus::RejectedStaleEpoch);
    NOSFIX_CHECK(normalizer.Epoch() == 8);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x1u);
}

// ---------------------------------------------------------------------------
// 5. Reset: one synthetic release per held lane, ordered, once
// ---------------------------------------------------------------------------

void TestResetSemantics() {
    std::printf("reset releases each held lane once\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 3> table{Bind(1, 1, 10, 0), Bind(2, 3, 7, 9),
                                            Bind(2, 3, 7, 4)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 7, true, 0.8f), Stamp(kQpcBase, 1, 0),
                             events) == InputApplyStatus::Applied);
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f), Stamp(kQpcBase, 1, 1),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    // Lanes were pressed out of order (9, 4, then 0); reset must sort them.
    NOSFIX_CHECK(normalizer.HeldLaneMask() == ((1u << 0) | (1u << 4) | (1u << 9)));
    NOSFIX_CHECK(normalizer.HeldControlCount() == 2);

    const auto reset_stamp = Stamp(kQpcBase + kQpcMs, 2, 0, 3);
    std::array<LogicalInputEvent, kCapacity> buffer{};
    FillSentinel(buffer);
    auto reset = normalizer.Reset(reset_stamp, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 3);
    NOSFIX_CHECK(reset.required_event_count == 3);
    EXPECT_EVENT(buffer[0], reset_stamp, 1, 1, 0, InputEdge::Release, 0.0f, true);
    EXPECT_EVENT(buffer[1], reset_stamp, 2, 3, 4, InputEdge::Release, 0.0f, true);
    EXPECT_EVENT(buffer[2], reset_stamp, 2, 3, 9, InputEdge::Release, 0.0f, true);
    NOSFIX_CHECK(buffer[3].lane == 0xFF); // no phantom events past event_count
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(normalizer.HeldControlCount() == 0);
    NOSFIX_CHECK(normalizer.Epoch() == 3);
    NOSFIX_CHECK(SameStamp(normalizer.ConsumedStamp(), reset_stamp));

    // Releasing again emits nothing: a lane is released exactly once.
    reset = normalizer.Reset(Stamp(kQpcBase + 2 * kQpcMs, 3, 0, 3), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(reset.required_event_count == 0);

    // Held state was really cleared: the next report is a fresh press.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f),
                             Stamp(kQpcBase + 3 * kQpcMs, 4, 0, 3), events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    events.clear();

    // A same-epoch reset is the repair path, but may not travel backwards.
    reset = normalizer.Reset(Stamp(kQpcBase, 1, 0, 3), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedBackwardStamp);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x1u);

    // A forward, same-epoch reset is accepted and releases once.
    reset = normalizer.Reset(Stamp(kQpcBase + 4 * kQpcMs, 5, 0, 3), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 1);
    EXPECT_EVENT(buffer[0], Stamp(kQpcBase + 4 * kQpcMs, 5, 0, 3), 1, 1, 0,
                 InputEdge::Release, 0.0f, true);
}

void TestResetCapacityRetry() {
    std::printf("reset capacity retry is transactional\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 3> table{Bind(1, 1, 10, 0), Bind(2, 3, 7, 4),
                                            Bind(2, 3, 7, 9)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 7, true, 0.8f), Stamp(kQpcBase, 1, 0),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f), Stamp(kQpcBase, 1, 1),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x211u); // lanes 0, 4, 9

    const auto reset_stamp = Stamp(kQpcBase + 10 * kQpcMs, 20, 0, 2);
    std::array<LogicalInputEvent, kCapacity> buffer{};
    FillSentinel(buffer);

    auto reset = normalizer.Reset(reset_stamp, std::span<LogicalInputEvent>(buffer.data(), 0));
    NOSFIX_CHECK(reset.status == InputResetStatus::InsufficientOutputCapacity);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(reset.required_event_count == 3);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x211u);
    NOSFIX_CHECK(normalizer.HeldControlCount() == 2);
    NOSFIX_CHECK(normalizer.Epoch() == 1);
    NOSFIX_CHECK(SentinelIntact(buffer));

    // A short retry of the identical reset fails identically: nothing moved.
    reset = normalizer.Reset(reset_stamp, std::span<LogicalInputEvent>(buffer.data(), 2));
    NOSFIX_CHECK(reset.status == InputResetStatus::InsufficientOutputCapacity);
    NOSFIX_CHECK(reset.required_event_count == 3);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0x211u);
    NOSFIX_CHECK(SentinelIntact(buffer));

    // No release has been emitted, so a held rebind is still refused.
    const std::array<InputBinding, 1> moved{Bind(1, 1, 10, 1)};
    NOSFIX_CHECK(normalizer.Configure(moved) == InputConfigStatus::RejectedHeldControlRebind);

    // The exact retry with enough room succeeds and adopts the epoch.
    reset = normalizer.Reset(reset_stamp, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 3);
    EXPECT_EVENT(buffer[0], reset_stamp, 1, 1, 0, InputEdge::Release, 0.0f, true);
    EXPECT_EVENT(buffer[1], reset_stamp, 2, 3, 4, InputEdge::Release, 0.0f, true);
    EXPECT_EVENT(buffer[2], reset_stamp, 2, 3, 9, InputEdge::Release, 0.0f, true);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(normalizer.Epoch() == 2);
    NOSFIX_CHECK(normalizer.Configure(moved) == InputConfigStatus::Accepted);
}

// ---------------------------------------------------------------------------
// 6. Device generations, capacity retry on Apply
// ---------------------------------------------------------------------------

void TestStaleDeviceGeneration() {
    std::printf("stale device generation is unmapped, not merged\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 1> generation_one{Bind(1, 1, 10, 0)};
    NOSFIX_CHECK(normalizer.Configure(generation_one) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.6f), Stamp(kQpcBase, 1),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(normalizer.IsLaneHeld(0));
    events.clear();

    // The same control id under a new generation is a different device: it must
    // not press, release or alias the old lane.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 2, 10, true, 0.6f),
                             Stamp(kQpcBase, 1, 1), events) ==
                 InputApplyStatus::IgnoredUnmappedControl);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 2, 10, false, 0.0f),
                             Stamp(kQpcBase, 1, 2), events) ==
                 InputApplyStatus::IgnoredUnmappedControl);
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(normalizer.IsLaneHeld(0)); // still held by generation 1

    // The original generation still releases properly.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, false, 0.0f),
                             Stamp(kQpcBase, 1, 3), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    events.clear();

    // Rebind to the reconnected generation only. Nothing is held, so this is
    // allowed without a reset.
    const std::array<InputBinding, 1> generation_two{Bind(1, 2, 10, 5)};
    NOSFIX_CHECK(normalizer.Configure(generation_two) == InputConfigStatus::Accepted);
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.6f),
                             Stamp(kQpcBase, 2), events) ==
                 InputApplyStatus::IgnoredUnmappedControl);
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 2, 10, true, 0.6f),
                             Stamp(kQpcBase, 3), events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 3), 1, 2, 5, InputEdge::Press, 0.6f, false);
    }
}

void TestApplyCapacityRetry() {
    std::printf("apply capacity retry consumes nothing\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 3> table{Bind(2, 3, 7, 4), Bind(2, 3, 7, 9),
                                            Bind(2, 3, 8, 27)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    const auto c_press = Stamp(kQpcBase, 1, 0);
    std::array<LogicalInputEvent, kCapacity> buffer{};
    FillSentinel(buffer);

    auto result = normalizer.Apply(Control(2, 3, 7, true, 0.7f), c_press,
                                   std::span<LogicalInputEvent>(buffer.data(), 0));
    NOSFIX_CHECK(result.status == InputApplyStatus::InsufficientOutputCapacity);
    NOSFIX_CHECK(result.event_count == 0);
    NOSFIX_CHECK(result.required_event_count == 2);
    NOSFIX_CHECK(!result.Consumed());
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(normalizer.HeldControlCount() == 0);
    NOSFIX_CHECK(!normalizer.HasAdoptedEpoch());
    NOSFIX_CHECK(SentinelIntact(buffer));

    // One slot is still not enough, and the identical retry must behave the
    // same way: nothing was consumed, so the stamp is not a duplicate now.
    result = normalizer.Apply(Control(2, 3, 7, true, 0.7f), c_press,
                              std::span<LogicalInputEvent>(buffer.data(), 1));
    NOSFIX_CHECK(result.status == InputApplyStatus::InsufficientOutputCapacity);
    NOSFIX_CHECK(result.required_event_count == 2);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(SentinelIntact(buffer));

    // The exact retry with two slots emits both edges in lane order.
    result = normalizer.Apply(Control(2, 3, 7, true, 0.7f), c_press,
                              std::span<LogicalInputEvent>(buffer.data(), 2));
    NOSFIX_CHECK(result.status == InputApplyStatus::Applied);
    NOSFIX_CHECK(result.event_count == 2);
    NOSFIX_CHECK(result.required_event_count == 2);
    EXPECT_EVENT(buffer[0], c_press, 2, 3, 4, InputEdge::Press, 0.7f, false);
    EXPECT_EVENT(buffer[1], c_press, 2, 3, 9, InputEdge::Press, 0.7f, false);
    NOSFIX_CHECK(buffer[2].lane == 0xFF); // untouched

    // A single-edge report fits exactly one slot.
    std::array<LogicalInputEvent, 1> one{};
    const auto d_press = Stamp(kQpcBase, 1, 1);
    result = normalizer.Apply(Control(2, 3, 8, true, 0.9f), d_press, one);
    NOSFIX_CHECK(result.status == InputApplyStatus::Applied);
    NOSFIX_CHECK(result.event_count == 1);
    EXPECT_EVENT(one[0], d_press, 2, 3, 27, InputEdge::Press, 0.9f, false);

    // No edge and no mapping need no room at all.
    result = normalizer.Apply(Control(2, 3, 7, true, 0.7f), Stamp(kQpcBase, 1, 2),
                              std::span<LogicalInputEvent>());
    NOSFIX_CHECK(result.status == InputApplyStatus::Applied);
    NOSFIX_CHECK(result.event_count == 0);
    result = normalizer.Apply(Control(9, 9, 9, true, 0.5f), Stamp(kQpcBase, 1, 3),
                              std::span<LogicalInputEvent>());
    NOSFIX_CHECK(result.status == InputApplyStatus::IgnoredUnmappedControl);

    // A press that needs one slot reports the requirement instead of emitting.
    const auto d_release = Stamp(kQpcBase, 1, 4);
    result = normalizer.Apply(Control(2, 3, 8, false, 0.0f), d_release,
                              std::span<LogicalInputEvent>());
    NOSFIX_CHECK(result.status == InputApplyStatus::InsufficientOutputCapacity);
    NOSFIX_CHECK(result.required_event_count == 1);
    NOSFIX_CHECK(normalizer.IsLaneHeld(27));
}

// Mixed multi-lane and shared-lane aggregation completes section 3.
void TestMixedLaneAggregation() {
    std::printf("mixed multi-lane and shared-lane aggregation\n");
    InputNormalizer normalizer;
    // Control 7 holds lanes 4 and 9; control 8 shares lane 4.
    const std::array<InputBinding, 3> table{Bind(2, 3, 7, 4), Bind(2, 3, 7, 9),
                                            Bind(2, 3, 8, 4)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 7, true, 0.7f), Stamp(kQpcBase, 1, 0),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 2);
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 8, true, 0.5f), Stamp(kQpcBase, 1, 1),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.empty()); // lane 4 already held
    NOSFIX_CHECK(normalizer.HeldLaneMask() == ((1u << 4) | (1u << 9)));

    // Releasing the two-lane control releases only the lane nobody else holds.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 7, false, 0.0f), Stamp(kQpcBase, 1, 2),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 1, 2), 2, 3, 9, InputEdge::Release, 0.0f,
                     false);
    }
    NOSFIX_CHECK(normalizer.IsLaneHeld(4));
    events.clear();

    // The remaining holder releases lane 4.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(2, 3, 8, false, 0.0f), Stamp(kQpcBase, 1, 3),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], Stamp(kQpcBase, 1, 3), 2, 3, 4, InputEdge::Release, 0.0f,
                     false);
    }
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
}

void TestResetCannotReopenConsumedIdentity() {
    std::printf("a reset never reopens a consumed identity\n");
    const std::array<InputBinding, 2> table{Bind(1, 1, 10, 0), Bind(1, 1, 11, 2)};
    InputNormalizer normalizer;
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::array<LogicalInputEvent, kCapacity> buffer{};
    std::vector<LogicalInputEvent> events;
    const auto s = Stamp(kQpcBase, 10, 0);

    // Apply(S) consumes S. Reset(S) must not mark S unclaimed again: doing so
    // would let the same physical press be delivered twice.
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f), s, events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    NOSFIX_CHECK(normalizer.IsLaneHeld(0));
    const auto cursor_before = normalizer.ConsumedStamp();
    FillSentinel(buffer);
    auto reset = normalizer.Reset(s, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(reset.required_event_count == 0);
    NOSFIX_CHECK(normalizer.IsLaneHeld(0)); // no synthetic release either
    NOSFIX_CHECK(normalizer.HeldControlCount() == 1);
    NOSFIX_CHECK(SameStamp(normalizer.ConsumedStamp(), cursor_before));
    NOSFIX_CHECK(SentinelIntact(buffer));

    // The consuming report still cannot be replayed after the refused reset.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f), s, events) ==
                 InputApplyStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(normalizer.IsLaneHeld(0));

    // Identity, not QPC: a different QPC on the same (sequence, ordinal) is the
    // same identity and still cannot be reopened.
    reset = normalizer.Reset(Stamp(kQpcBase + 5 * kQpcMs, 10, 0), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(normalizer.IsLaneHeld(0));

    // A strictly newer identity still resets and releases once.
    const auto newer = Stamp(kQpcBase + 6 * kQpcMs, 11, 0);
    reset = normalizer.Reset(newer, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 1);
    EXPECT_EVENT(buffer[0], newer, 1, 1, 0, InputEdge::Release, 0.0f, true);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);

    // Reset -> Apply -> Reset(same) -> Apply(same): the marker cannot reopen it.
    InputNormalizer second;
    NOSFIX_CHECK(second.Configure(table) == InputConfigStatus::Accepted);
    const auto t = Stamp(kQpcBase, 20, 0);
    reset = second.Reset(t, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 0);
    events.clear();
    NOSFIX_CHECK(ApplyReport(second, Control(1, 1, 11, true, 0.5f), t, events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    const auto second_cursor = second.ConsumedStamp();
    FillSentinel(buffer);
    reset = second.Reset(t, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(reset.required_event_count == 0);
    NOSFIX_CHECK(SentinelIntact(buffer));
    NOSFIX_CHECK(SameStamp(second.ConsumedStamp(), second_cursor));
    NOSFIX_CHECK(second.IsLaneHeld(2));
    events.clear();
    NOSFIX_CHECK(ApplyReport(second, Control(1, 1, 11, false, 0.0f), t, events) ==
                 InputApplyStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(second.IsLaneHeld(2));
    reset = second.Reset(Stamp(kQpcBase + kQpcMs, 21, 0), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 1);
    NOSFIX_CHECK(second.HeldLaneMask() == 0);

    // Repeated Reset before any Apply is an idempotent boundary refresh, and the
    // first report may still claim the identity exactly once.
    InputNormalizer third;
    NOSFIX_CHECK(third.Configure(table) == InputConfigStatus::Accepted);
    const auto u = Stamp(kQpcBase, 30, 0);
    reset = third.Reset(u, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 0);
    reset = third.Reset(u, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 0);
    FillSentinel(buffer);
    reset = third.Reset(u, buffer); // identical marker, still unclaimed
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(reset.required_event_count == 0);
    NOSFIX_CHECK(SentinelIntact(buffer)); // idempotent: no output written
    reset = third.Reset(Stamp(kQpcBase + 2 * kQpcMs, 30, 0), buffer); // same identity
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedInvalidStamp);
    NOSFIX_CHECK(SameStamp(third.ConsumedStamp(), u));
    NOSFIX_CHECK(SentinelIntact(buffer));
    reset = third.Reset(Stamp(kQpcBase - kQpcMs, 30, 0), buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedInvalidStamp);
    NOSFIX_CHECK(SameStamp(third.ConsumedStamp(), u));
    NOSFIX_CHECK(SentinelIntact(buffer));
    for (const auto qpc : {kQpcBase - kQpcMs, kQpcBase + kQpcMs}) {
        const auto invalid = third.Apply(Control(1, 1, 10, true, 0.5f),
                                         Stamp(qpc, 30, 0), buffer);
        NOSFIX_CHECK(invalid.status == InputApplyStatus::RejectedInvalidStamp);
        NOSFIX_CHECK(SameStamp(third.ConsumedStamp(), u));
        NOSFIX_CHECK(SentinelIntact(buffer));
        NOSFIX_CHECK(third.HeldLaneMask() == 0);
    }
    events.clear();
    NOSFIX_CHECK(ApplyReport(third, Control(1, 1, 10, true, 0.5f), u, events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    // Spent for good now.
    const auto third_cursor = third.ConsumedStamp();
    FillSentinel(buffer);
    reset = third.Reset(u, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(reset.required_event_count == 0);
    NOSFIX_CHECK(SentinelIntact(buffer));
    NOSFIX_CHECK(SameStamp(third.ConsumedStamp(), third_cursor));
    NOSFIX_CHECK(third.IsLaneHeld(0));

    // Newer-identity reset with an insufficient-capacity retry; afterwards the
    // marker is claimable, and retrying the successful reset stays idempotent.
    InputNormalizer fourth;
    NOSFIX_CHECK(fourth.Configure(table) == InputConfigStatus::Accepted);
    events.clear();
    NOSFIX_CHECK(ApplyReport(fourth, Control(1, 1, 10, true, 0.5f), Stamp(kQpcBase, 40, 0),
                             events) == InputApplyStatus::Applied);
    NOSFIX_CHECK(ApplyReport(fourth, Control(1, 1, 11, true, 0.5f), Stamp(kQpcBase, 40, 1),
                             events) == InputApplyStatus::Applied);
    const auto retry_stamp = Stamp(kQpcBase + kQpcMs, 41, 0);
    const auto fourth_cursor = fourth.ConsumedStamp();
    FillSentinel(buffer);
    reset = fourth.Reset(retry_stamp, std::span<LogicalInputEvent>(buffer.data(), 1));
    NOSFIX_CHECK(reset.status == InputResetStatus::InsufficientOutputCapacity);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(reset.required_event_count == 2);
    NOSFIX_CHECK(SentinelIntact(buffer));
    NOSFIX_CHECK(SameStamp(fourth.ConsumedStamp(), fourth_cursor));
    NOSFIX_CHECK(fourth.HeldLaneMask() == ((1u << 0) | (1u << 2)));
    NOSFIX_CHECK(fourth.Epoch() == 1);
    reset = fourth.Reset(retry_stamp, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 2);
    EXPECT_EVENT(buffer[0], retry_stamp, 1, 1, 0, InputEdge::Release, 0.0f, true);
    EXPECT_EVENT(buffer[1], retry_stamp, 1, 1, 2, InputEdge::Release, 0.0f, true);
    NOSFIX_CHECK(buffer[2].lane == 0xFF); // beyond event_count untouched
    FillSentinel(buffer);
    reset = fourth.Reset(retry_stamp, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(SentinelIntact(buffer));
    events.clear();
    NOSFIX_CHECK(ApplyReport(fourth, Control(1, 1, 11, true, 0.6f), retry_stamp, events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
}

// ---------------------------------------------------------------------------
// 7. Boundary-marker identity, allocation freedom, table saturation
// ---------------------------------------------------------------------------

void TestResetBoundaryIdentity() {
    std::printf("reset stamp is a boundary marker, not a consumed report\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 2> table{Bind(1, 1, 10, 0), Bind(1, 1, 11, 2)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::vector<LogicalInputEvent> events;
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 10, true, 0.5f), Stamp(kQpcBase, 1, 0),
                             events) == InputApplyStatus::Applied);

    // Caller protocol: reset on the first report of the new stream, then feed
    // exactly that report. Neither call may reject the other.
    const auto boundary = Stamp(kQpcBase + kQpcMs, 7, 0, 2);
    std::array<LogicalInputEvent, kCapacity> buffer{};
    auto reset = normalizer.Reset(boundary, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 1); // lane 0 released synthetically
    EXPECT_EVENT(buffer[0], boundary, 1, 1, 0, InputEdge::Release, 0.0f, true);

    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 11, true, 0.6f), boundary, events) ==
                 InputApplyStatus::Applied);
    NOSFIX_CHECK(events.size() == 1);
    if (events.size() == 1) {
        EXPECT_EVENT(events[0], boundary, 1, 1, 2, InputEdge::Press, 0.6f, false);
    }
    // The identity is spent now: a repeat is a duplicate, not a new press, and
    // a reset naming it cannot reopen it either.
    events.clear();
    NOSFIX_CHECK(ApplyReport(normalizer, Control(1, 1, 11, false, 0.0f), boundary, events) ==
                 InputApplyStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(events.empty());
    NOSFIX_CHECK(normalizer.IsLaneHeld(2));
    const auto boundary_cursor = normalizer.ConsumedStamp();
    FillSentinel(buffer);
    reset = normalizer.Reset(boundary, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::RejectedDuplicateStamp);
    NOSFIX_CHECK(reset.event_count == 0);
    NOSFIX_CHECK(reset.required_event_count == 0);
    NOSFIX_CHECK(SentinelIntact(buffer));
    NOSFIX_CHECK(SameStamp(normalizer.ConsumedStamp(), boundary_cursor));
    NOSFIX_CHECK(normalizer.IsLaneHeld(2));

    // A newer epoch restarts the ordering domain, so its QPC need not be
    // greater than the abandoned stream's. The stale-epoch rule still holds.
    const auto newer = Stamp(0, 1, 0, 3);
    reset = normalizer.Reset(newer, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == 1); // lane 2 released
    EXPECT_EVENT(buffer[0], newer, 1, 1, 2, InputEdge::Release, 0.0f, true);
    NOSFIX_CHECK(normalizer.Epoch() == 3);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
}

void TestNoAllocationInRealtimePath() {
    std::printf("Apply/Reset perform no allocation\n");
    InputNormalizer normalizer;
    const std::array<InputBinding, 3> table{Bind(2, 3, 7, 4), Bind(2, 3, 7, 9),
                                            Bind(2, 3, 8, 27)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::array<LogicalInputEvent, kCapacity> buffer{};
    const auto press = Stamp(kQpcBase, 1, 0);
    NOSFIX_CHECK(AllocationsDuring([&] {
                     const auto result = normalizer.Apply(Control(2, 3, 7, true, 0.7f), press,
                                                          buffer);
                     NOSFIX_CHECK(result.event_count == 2);
                 }) == 0);
    // Typematic, unmapped, rejected and capacity-short paths are allocation
    // free too: the realtime caller must never hit the heap.
    NOSFIX_CHECK(AllocationsDuring([&] {
                     const auto typematic = normalizer.Apply(Control(2, 3, 7, true, 0.7f),
                                                             Stamp(kQpcBase, 1, 1), buffer);
                     NOSFIX_CHECK(typematic.status == InputApplyStatus::Applied);
                     NOSFIX_CHECK(typematic.event_count == 0);
                 }) == 0);
    NOSFIX_CHECK(AllocationsDuring([&] {
                     const auto unmapped = normalizer.Apply(Control(9, 9, 9, true, 0.5f),
                                                            Stamp(kQpcBase, 1, 2), buffer);
                     NOSFIX_CHECK(unmapped.status == InputApplyStatus::IgnoredUnmappedControl);
                 }) == 0);
    NOSFIX_CHECK(AllocationsDuring([&] {
                     const auto rejected = normalizer.Apply(Control(0, 3, 7, true, 0.5f),
                                                            Stamp(kQpcBase, 1, 3), buffer);
                     NOSFIX_CHECK(rejected.status == InputApplyStatus::RejectedInvalidReport);
                 }) == 0);
    NOSFIX_CHECK(AllocationsDuring([&] {
                     const auto short_capacity = normalizer.Apply(
                         Control(2, 3, 7, false, 0.0f), Stamp(kQpcBase, 1, 4),
                         std::span<LogicalInputEvent>(buffer.data(), 1));
                     NOSFIX_CHECK(short_capacity.status ==
                                  InputApplyStatus::InsufficientOutputCapacity);
                 }) == 0);
    NOSFIX_CHECK(AllocationsDuring([&] {
                     const auto two_lane = normalizer.Apply(Control(2, 3, 7, false, 0.0f),
                                                            Stamp(kQpcBase, 1, 4), buffer);
                     NOSFIX_CHECK(two_lane.status == InputApplyStatus::Applied);
                     NOSFIX_CHECK(two_lane.event_count == 2);
                 }) == 0);
    NOSFIX_CHECK(AllocationsDuring([&] {
                     const auto reset = normalizer.Reset(Stamp(kQpcBase + kQpcMs, 2, 0, 2),
                                                         buffer);
                     NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
                 }) == 0);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
}

void TestBindingBoundSaturation() {
    std::printf("maximum binding table saturates safely\n");
    // The whole state is a fixed table; it must stay embeddable in the plugin.
    static_assert(sizeof(InputNormalizer) <= 16 * 1024,
                  "InputNormalizer state must stay small enough to embed");
    InputNormalizer normalizer;
    std::vector<InputBinding> table;
    table.reserve(nosfix::kInputMaxBindings);
    for (std::size_t i = 0; i < nosfix::kInputMaxBindings; ++i) {
        table.push_back(Bind(1, 1, static_cast<std::uint32_t>(i + 1),
                             static_cast<std::uint8_t>(i % nosfix::kInputLaneCount)));
    }
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    std::array<LogicalInputEvent, kCapacity> buffer{};
    std::size_t presses = 0;
    for (std::size_t i = 0; i < nosfix::kInputMaxBindings; ++i) {
        const auto applied = normalizer.Apply(
            Control(1, 1, static_cast<std::uint32_t>(i + 1), true, 0.5f),
            Stamp(kQpcBase, static_cast<std::uint64_t>(i + 1), 0), buffer);
        NOSFIX_CHECK(applied.status == InputApplyStatus::Applied);
        presses += applied.event_count;
    }
    // Every bound control is held at once: the control table is full, and each
    // of the 28 lanes was pressed exactly once.
    NOSFIX_CHECK(normalizer.HeldControlCount() == nosfix::kInputMaxBindings);
    NOSFIX_CHECK(normalizer.HeldLaneMask() == ((1u << nosfix::kInputLaneCount) - 1u));
    NOSFIX_CHECK(presses == nosfix::kInputLaneCount);

    // Reset must release all 28 lanes once, in lane order.
    const auto reset_stamp = Stamp(kQpcBase + kQpcMs, 1000, 0, 2);
    const auto reset = normalizer.Reset(reset_stamp, buffer);
    NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
    NOSFIX_CHECK(reset.event_count == nosfix::kInputLaneCount);
    for (std::uint8_t lane = 0; lane < nosfix::kInputLaneCount; ++lane) {
        NOSFIX_CHECK(buffer[lane].lane == lane);
        NOSFIX_CHECK(buffer[lane].edge == InputEdge::Release);
        NOSFIX_CHECK(buffer[lane].synthetic);
        NOSFIX_CHECK(buffer[lane].velocity == 0.0f);
    }
    NOSFIX_CHECK(normalizer.HeldLaneMask() == 0);
    NOSFIX_CHECK(normalizer.HeldControlCount() == 0);
}

// ---------------------------------------------------------------------------
// 8. Replay: identical edges under 60..360 Hz consumer schedules
// ---------------------------------------------------------------------------

struct ReplayReport final {
    InputStamp stamp;
    PhysicalControlState state;
};

constexpr std::int64_t kReplayBaseQpc = 1'000'000'000'000LL;
constexpr int kReplayMilliseconds = 4000;
constexpr int kEpochChangeMs = 2500;
constexpr int kStallBeginMs = 2000;
constexpr int kStallMs = 1530; // the measured scene-loading stall

std::vector<ReplayReport> BuildReplayStream() {
    struct Pending final {
        std::uint64_t source;
        std::uint64_t generation;
        std::uint32_t control;
        bool down;
        float velocity;
    };

    std::vector<ReplayReport> reports;
    reports.reserve(static_cast<std::size_t>(kReplayMilliseconds) * 2);
    std::uint64_t sequence = 0;
    bool a_down = false;
    float a_velocity = 0.0f;

    for (int ms = 0; ms < kReplayMilliseconds; ++ms) {
        std::vector<Pending> block;
        const std::uint64_t epoch = ms < kEpochChangeMs ? 1u : 2u;
        if (epoch == 1) {
            if (ms == 10) block.push_back({1, 1, 10, true, 0.60f});
            if (ms >= 60 && ms <= 400 && (ms % 50) == 0) {
                block.push_back({1, 1, 10, true, 0.60f}); // typematic
            }
            if (ms == 200) block.push_back({1, 1, 10, true, 0.85f}); // velocity only
            if (ms == 300) block.push_back({1, 1, 11, true, 0.50f}); // no lane edge
            if (ms == 350) block.push_back({1, 1, 11, false, 0.0f}); // no lane edge
            if (ms == 410) block.push_back({1, 1, 10, false, 0.0f});
            if (ms == 500) {
                block.push_back({2, 3, 7, true, 0.70f});
                block.push_back({2, 3, 8, true, 0.90f}); // chord: same QPC
            }
            if (ms == 520) {
                block.push_back({2, 3, 7, false, 0.0f});
                block.push_back({2, 3, 8, false, 0.0f});
            }
            if (ms == 600) block.push_back({1, 1, 11, true, 0.50f});
            if (ms == 620) block.push_back({1, 1, 11, false, 0.0f});
            if (ms == 1000) block.push_back({1, 1, 10, true, 0.40f});
            if (ms == 1002) block.push_back({1, 1, 10, false, 0.0f}); // down/up in one 60 Hz frame
            if (ms == 1500) {
                block.push_back({1, 1, 10, true, 0.50f});
                block.push_back({1, 1, 11, true, 0.55f});
                block.push_back({2, 3, 7, true, 0.65f});
                block.push_back({2, 3, 8, true, 0.75f}); // four-report batch
            }
            if (ms == 1520) {
                block.push_back({1, 1, 10, false, 0.0f});
                block.push_back({1, 1, 11, false, 0.0f});
                block.push_back({2, 3, 7, false, 0.0f});
                block.push_back({2, 3, 8, false, 0.0f});
            }
            if (ms == 2400) {
                block.push_back({2, 3, 7, true, 0.80f});
                block.push_back({2, 3, 8, true, 0.85f}); // held across the loss
            }
            if (ms == 2450) block.push_back({2, 3, 7, true, 0.90f});
        } else {
            if (ms == 2500) {
                block.push_back({2, 3, 7, true, 0.80f}); // post-reconnect snapshot
                block.push_back({2, 3, 8, true, 0.85f});
            }
            if (ms == 2520) block.push_back({1, 1, 11, true, 0.50f});
            if (ms == 2521) block.push_back({1, 1, 11, false, 0.0f});
            if (ms == 2700) {
                block.push_back({2, 3, 7, false, 0.0f});
                block.push_back({2, 3, 8, false, 0.0f});
            }
            if (ms == 2800) block.push_back({1, 1, 10, true, 0.30f});
            if (ms == 2801) block.push_back({1, 1, 10, false, 0.0f});
            if (ms == 3000) block.push_back({2, 3, 8, true, 0.70f});
            if (ms == 3001) block.push_back({2, 3, 8, false, 0.0f});
            if (ms == 3500) block.push_back({2, 3, 7, true, 0.60f});
            if (ms == 3502) block.push_back({2, 3, 7, false, 0.0f});
        }

        // Track control A so the keep-alive repeats the current level.
        for (const Pending& pending : block) {
            if (pending.source == 1 && pending.control == 10) {
                a_down = pending.down;
                a_velocity = pending.velocity;
            }
        }
        // Real devices report at about 1 kHz even when nothing changes.
        block.push_back({1, 1, 10, a_down, a_velocity});

        ++sequence;
        for (std::size_t i = 0; i < block.size(); ++i) {
            ReplayReport report;
            report.stamp.qpc = kReplayBaseQpc + static_cast<std::int64_t>(ms) * kQpcMs;
            report.stamp.sequence = sequence;
            report.stamp.stream_epoch = epoch;
            report.stamp.ordinal = static_cast<std::uint32_t>(i);
            report.state.source_id = block[i].source;
            report.state.device_generation = block[i].generation;
            report.state.control_id = block[i].control;
            report.state.down = block[i].down;
            report.state.velocity = block[i].velocity;
            reports.push_back(report);
        }
    }
    return reports;
}

struct ScheduleResult final {
    std::vector<LogicalInputEvent> events;
    std::size_t frames = 0;
    std::size_t retries = 0;
    std::size_t max_batch_reports = 0;
};

// Drains every report whose QPC is at or before `now`, in capture order. The
// caller's buffer is deliberately one slot: multi-lane reports exercise the
// capacity-retry contract on every schedule.
void DrainReports(InputNormalizer& normalizer, const std::vector<ReplayReport>& reports,
                  std::size_t& next, std::int64_t now, std::uint64_t& epoch,
                  ScheduleResult& result) {
    std::size_t batch = 0;
    while (next < reports.size() && reports[next].stamp.qpc <= now) {
        const ReplayReport& report = reports[next];

        if (report.stamp.stream_epoch != epoch) {
            std::array<LogicalInputEvent, 1> small{};
            auto reset = normalizer.Reset(report.stamp, small);
            if (reset.status == InputResetStatus::InsufficientOutputCapacity) {
                ++result.retries;
                std::array<LogicalInputEvent, kCapacity> large{};
                reset = normalizer.Reset(report.stamp, large);
                NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
                NOSFIX_CHECK(reset.event_count == 3); // lanes 4, 9, 27
                for (std::size_t i = 0; i < reset.event_count; ++i) {
                    result.events.push_back(large[i]);
                }
            } else {
                NOSFIX_CHECK(reset.status == InputResetStatus::Applied);
                NOSFIX_CHECK(reset.event_count == 0);
            }
            epoch = report.stamp.stream_epoch;
        }

        std::array<LogicalInputEvent, 1> small{};
        auto applied = normalizer.Apply(report.state, report.stamp, small);
        if (applied.status == InputApplyStatus::InsufficientOutputCapacity) {
            ++result.retries;
            std::array<LogicalInputEvent, kCapacity> large{};
            applied = normalizer.Apply(report.state, report.stamp, large);
            NOSFIX_CHECK(applied.status == InputApplyStatus::Applied);
            NOSFIX_CHECK(applied.event_count == 2); // the only 2-lane control
            for (std::size_t i = 0; i < applied.event_count; ++i) {
                result.events.push_back(large[i]);
            }
        } else {
            NOSFIX_CHECK(applied.status == InputApplyStatus::Applied ||
                         applied.status == InputApplyStatus::IgnoredUnmappedControl);
            for (std::size_t i = 0; i < applied.event_count; ++i) {
                result.events.push_back(small[i]);
            }
        }

        ++next;
        ++batch;
    }
    if (batch > result.max_batch_reports) result.max_batch_reports = batch;
}

ScheduleResult RunSchedule(const std::vector<ReplayReport>& reports, unsigned rate) {
    InputNormalizer normalizer;
    const std::array<InputBinding, 5> table{
        Bind(1, 1, 10, 0), Bind(1, 1, 11, 0), Bind(2, 3, 7, 4), Bind(2, 3, 7, 9),
        Bind(2, 3, 8, 27)};
    NOSFIX_CHECK(normalizer.Configure(table) == InputConfigStatus::Accepted);

    ScheduleResult result;
    result.events.reserve(64);
    std::size_t next = 0;
    std::uint64_t epoch = reports.front().stamp.stream_epoch;

    const std::int64_t stall_begin =
        kReplayBaseQpc + static_cast<std::int64_t>(kStallBeginMs) * kQpcMs;
    const std::int64_t stall_end = stall_begin + static_cast<std::int64_t>(kStallMs) * kQpcMs;
    const std::int64_t end =
        kReplayBaseQpc + static_cast<std::int64_t>(kReplayMilliseconds + 400) * kQpcMs;

    std::int64_t tick = kReplayBaseQpc;
    std::int64_t remainder = 0;
    std::int64_t last_wake = kReplayBaseQpc;
    while (tick < end) {
        remainder += kQpcFrequency;
        tick += remainder / static_cast<std::int64_t>(rate);
        remainder %= static_cast<std::int64_t>(rate);

        const auto jitter =
            static_cast<std::int64_t>((result.frames * 7919U) % 400U) - 200;
        std::int64_t wake = tick + jitter;
        if (wake >= stall_begin && wake < stall_end) {
            // Presentation stall: the consumer does not run until it recovers.
            tick = stall_end;
            remainder = 0;
            wake = stall_end;
        }
        if (wake < last_wake) wake = last_wake;
        last_wake = wake;

        DrainReports(normalizer, reports, next, wake, epoch, result);
        ++result.frames;
    }
    DrainReports(normalizer, reports, next, std::numeric_limits<std::int64_t>::max(), epoch,
                 result);
    NOSFIX_CHECK(next == reports.size());
    return result;
}

void TestReplayScheduleInvariance() {
    std::printf("replay: identical edges at 60/90/120/144/165/240/360 Hz\n");
    const std::vector<ReplayReport> reports = BuildReplayStream();
    NOSFIX_CHECK(!reports.empty());
    std::printf("  stream: %zu reports over %d ms\n", reports.size(), kReplayMilliseconds);

    const ScheduleResult reference = RunSchedule(reports, 60);
    NOSFIX_CHECK(reference.events.size() == 42);

    std::size_t presses = 0;
    std::size_t releases = 0;
    std::size_t synthetic = 0;
    std::array<std::size_t, nosfix::kInputLaneCount> lane_presses{};
    std::array<std::size_t, nosfix::kInputLaneCount> lane_releases{};
    for (const LogicalInputEvent& event : reference.events) {
        NOSFIX_CHECK(event.lane < nosfix::kInputLaneCount);
        if (event.edge == InputEdge::Press) {
            ++presses;
            ++lane_presses[event.lane];
            NOSFIX_CHECK(event.velocity > 0.0f && event.velocity <= 1.0f);
            NOSFIX_CHECK(!event.synthetic);
        } else {
            ++releases;
            ++lane_releases[event.lane];
            NOSFIX_CHECK(event.velocity == 0.0f);
            if (event.synthetic) ++synthetic;
        }
    }
    NOSFIX_CHECK(presses == 21);
    NOSFIX_CHECK(releases == 21);
    NOSFIX_CHECK(synthetic == 3);
    // Lane 0: presses at 10 ms (A), 600 (B), 1000 (A), 1500 (A; B joins a held
    // lane), 2520 (B, epoch 2) and 2800 (A). The 1520 batch releases the lane
    // once, on B's release.
    NOSFIX_CHECK(lane_presses[0] == 6 && lane_releases[0] == 6);
    NOSFIX_CHECK(lane_presses[4] == 5 && lane_releases[4] == 5);
    NOSFIX_CHECK(lane_presses[9] == 5 && lane_releases[9] == 5);
    NOSFIX_CHECK(lane_presses[27] == 5 && lane_releases[27] == 5);
    std::printf("  reference: %zu events (%zu press, %zu release, %zu synthetic), %zu frames\n",
                reference.events.size(), presses, releases, synthetic, reference.frames);

    // The 1.53 s stall really happened, and the retry contract was exercised.
    NOSFIX_CHECK(reference.max_batch_reports > 1400);
    NOSFIX_CHECK(reference.retries > 0);
    NOSFIX_CHECK(reference.frames > 150);

    for (const unsigned rate : {90U, 120U, 144U, 165U, 240U, 360U}) {
        const ScheduleResult replay = RunSchedule(reports, rate);
        NOSFIX_CHECK(replay.events.size() == reference.events.size());
        NOSFIX_CHECK(replay.max_batch_reports > 1400);
        NOSFIX_CHECK(replay.retries > 0);
        bool identical = replay.events.size() == reference.events.size();
        const std::size_t common =
            replay.events.size() < reference.events.size() ? replay.events.size()
                                                           : reference.events.size();
        for (std::size_t i = 0; i < common; ++i) {
            if (!SameEvent(replay.events[i], reference.events[i])) identical = false;
        }
        NOSFIX_CHECK(identical);
    }
}

} // namespace

int main() {
    TestConfigurationValidation();
    TestHeldControlRebindIsRejected();
    TestEdgesAndRedundancy();
    TestChordsAndMultipleMappings();
    TestSharedLaneAggregation();
    TestUnmappedControl();
    TestStampValidation();
    TestEpochs();
    TestResetSemantics();
    TestResetCapacityRetry();
    TestStaleDeviceGeneration();
    TestApplyCapacityRetry();
    TestMixedLaneAggregation();
    TestResetBoundaryIdentity();
    TestResetCannotReopenConsumedIdentity();
    TestNoAllocationInRealtimePath();
    TestBindingBoundSaturation();
    TestReplayScheduleInvariance();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
