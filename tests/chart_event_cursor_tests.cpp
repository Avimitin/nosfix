// Tests for nosfix::ChartEventCursor. Always active, including Release builds.
// No sleeps, no threads, no external services. Deterministic.

#include "nosfix/chart_event_cursor.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <new>
#include <optional>
#include <random>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
std::atomic<std::size_t> g_allocation_count{0};
std::atomic<int> g_fail_after_allocation{-1};
} // namespace

// Replace the global allocator so hot-path allocation freedom can be measured
// and Configure()'s strong guarantee can be exercised under std::bad_alloc.
void* operator new(std::size_t size) {
    g_allocation_count.fetch_add(1, std::memory_order_relaxed);
    const int remaining = g_fail_after_allocation.load(std::memory_order_relaxed);
    if (remaining == 0) {
        throw std::bad_alloc();
    }
    if (remaining > 0) {
        g_fail_after_allocation.store(remaining - 1, std::memory_order_relaxed);
    }
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) throw std::bad_alloc();
    return memory;
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* memory) noexcept { ::operator delete(memory); }
void operator delete[](void* memory, std::size_t) noexcept { ::operator delete(memory); }

namespace {

using nosfix::ChartCursorStatus;
using nosfix::ChartEvent;
using nosfix::ChartEventCursor;
using nosfix::ChartEventDrain;
using nosfix::ChartTableStatus;
using nosfix::SongTime;

// Compile-time contracts: hot-path calls are noexcept, the cursor is neither
// copyable nor movable, and delivered events remain trivially copyable.
static_assert(std::is_trivially_copyable_v<ChartEvent>);
static_assert(std::is_trivially_copyable_v<ChartEventDrain>);
static_assert(std::is_default_constructible_v<ChartEventCursor>);
static_assert(!std::is_copy_constructible_v<ChartEventCursor>);
static_assert(!std::is_copy_assignable_v<ChartEventCursor>);
static_assert(!std::is_move_constructible_v<ChartEventCursor>);
static_assert(!std::is_move_assignable_v<ChartEventCursor>);
static_assert(noexcept(std::declval<ChartEventCursor&>().Reset(1, 0.0)));
static_assert(noexcept(std::declval<ChartEventCursor&>().Drain(
    SongTime{}, std::span<ChartEvent>())));
static_assert(noexcept(std::declval<const ChartEventCursor&>().CompletedThrough()));
static_assert(noexcept(std::declval<const ChartEventCursor&>().NextEvent()));

int g_checks = 0;
int g_failures = 0;
const char* g_test = "";

void Fail(const char* file, int line, const char* what) {
    ++g_failures;
    std::printf("  FAIL [%s] %s:%d: %s\n", g_test, file, line, what);
}

#define CHECK(expr)                                                            \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(expr)) Fail(__FILE__, __LINE__, #expr);                          \
    } while (false)

void CheckCursorStatusImpl(ChartCursorStatus actual, ChartCursorStatus expected,
                           const char* expression, const char* file, int line) {
    ++g_checks;
    if (actual == expected) return;
    ++g_failures;
    std::printf("  FAIL [%s] %s:%d: %s -> status %u, expected %u\n", g_test,
                file, line, expression, static_cast<unsigned>(actual),
                static_cast<unsigned>(expected));
}

void CheckTableStatusImpl(ChartTableStatus actual, ChartTableStatus expected,
                          const char* expression, const char* file, int line) {
    ++g_checks;
    if (actual == expected) return;
    ++g_failures;
    std::printf("  FAIL [%s] %s:%d: %s -> table status %u, expected %u\n",
                g_test, file, line, expression, static_cast<unsigned>(actual),
                static_cast<unsigned>(expected));
}

#define CHECK_CURSOR_STATUS(expr, expected)                                    \
    CheckCursorStatusImpl((expr), (expected), #expr, __FILE__, __LINE__)
#define CHECK_TABLE_STATUS(expr, expected)                                     \
    CheckTableStatusImpl((expr), (expected), #expr, __FILE__, __LINE__)

void CheckIdSequenceImpl(const std::vector<std::uint64_t>& actual,
                         const std::vector<std::uint64_t>& expected,
                         const char* label, const char* file, int line) {
    ++g_checks;
    if (actual == expected) return;
    ++g_failures;
    std::printf("  FAIL [%s] %s:%d: %s: %zu ids, expected %zu\n", g_test, file,
                line, label, actual.size(), expected.size());
    const std::size_t common = std::min(actual.size(), expected.size());
    for (std::size_t i = 0; i < common; ++i) {
        if (actual[i] != expected[i]) {
            std::printf("    first mismatch at %zu: id %llu, expected %llu\n", i,
                        static_cast<unsigned long long>(actual[i]),
                        static_cast<unsigned long long>(expected[i]));
            break;
        }
    }
}

#define CHECK_IDS(actual, expected)                                            \
    CheckIdSequenceImpl((actual), (expected), #actual, __FILE__, __LINE__)

ChartEvent At(std::uint64_t id, double song_us, std::uint32_t kind = 0,
              std::uint32_t payload = 0) {
    ChartEvent event;
    event.id = id;
    event.song_us = song_us;
    event.kind = kind;
    event.payload = payload;
    return event;
}

struct CursorSnapshot final {
    bool configured = false;
    bool started = false;
    std::uint64_t generation = 0;
    std::uint64_t last_reset_generation = 0;
    std::size_t event_count = 0;
    std::size_t pending = 0;
    bool has_watermark = false;
    double watermark_us = 0.0;
    std::uint64_t watermark_generation = 0;
    bool has_observed = false;
    double observed_us = 0.0;
    bool has_next = false;
    std::uint64_t next_id = 0;
    double next_us = 0.0;
};

CursorSnapshot Snap(const ChartEventCursor& cursor) {
    CursorSnapshot snapshot;
    snapshot.configured = cursor.IsConfigured();
    snapshot.started = cursor.IsStarted();
    snapshot.generation = cursor.ActiveGeneration();
    snapshot.last_reset_generation = cursor.LastResetGeneration();
    snapshot.event_count = cursor.EventCount();
    snapshot.pending = cursor.PendingCount();
    if (const auto watermark = cursor.CompletedThrough()) {
        snapshot.has_watermark = true;
        snapshot.watermark_us = watermark->microseconds;
        snapshot.watermark_generation = watermark->generation;
    }
    if (const auto observed = cursor.LastObserved()) {
        snapshot.has_observed = true;
        snapshot.observed_us = observed->microseconds;
    }
    if (const ChartEvent* next = cursor.NextEvent()) {
        snapshot.has_next = true;
        snapshot.next_id = next->id;
        snapshot.next_us = next->song_us;
    }
    return snapshot;
}

bool SameSnapshot(const CursorSnapshot& lhs, const CursorSnapshot& rhs) {
    return lhs.configured == rhs.configured && lhs.started == rhs.started &&
           lhs.generation == rhs.generation &&
           lhs.last_reset_generation == rhs.last_reset_generation &&
           lhs.event_count == rhs.event_count && lhs.pending == rhs.pending &&
           lhs.has_watermark == rhs.has_watermark &&
           lhs.watermark_us == rhs.watermark_us &&
           lhs.watermark_generation == rhs.watermark_generation &&
           lhs.has_observed == rhs.has_observed &&
           lhs.observed_us == rhs.observed_us && lhs.has_next == rhs.has_next &&
           lhs.next_id == rhs.next_id && lhs.next_us == rhs.next_us;
}

void CheckSameSnapshotImpl(const CursorSnapshot& actual,
                           const CursorSnapshot& expected, const char* label,
                           const char* file, int line) {
    ++g_checks;
    if (SameSnapshot(actual, expected)) return;
    ++g_failures;
    std::printf(
        "  FAIL [%s] %s:%d: %s: state changed\n"
        "    configured %d/%d started %d/%d gen %llu/%llu floor %llu/%llu "
        "count %zu/%zu pending %zu/%zu\n"
        "    watermark %d %.6f/%d %.6f observed %d %.6f/%d %.6f next %d "
        "%llu@%.6f/%d %llu@%.6f\n",
        g_test, file, line, label, actual.configured, expected.configured,
        actual.started, expected.started,
        static_cast<unsigned long long>(actual.generation),
        static_cast<unsigned long long>(expected.generation),
        static_cast<unsigned long long>(actual.last_reset_generation),
        static_cast<unsigned long long>(expected.last_reset_generation),
        actual.event_count,
        expected.event_count, actual.pending, expected.pending,
        actual.has_watermark, actual.watermark_us, expected.has_watermark,
        expected.watermark_us, actual.has_observed, actual.observed_us,
        expected.has_observed, expected.observed_us, actual.has_next,
        static_cast<unsigned long long>(actual.next_id), actual.next_us,
        expected.has_next,
        static_cast<unsigned long long>(expected.next_id), expected.next_us);
}

#define CHECK_SAME_SNAPSHOT(actual, expected)                                  \
    CheckSameSnapshotImpl((actual), (expected), #actual, __FILE__, __LINE__)

// ---------------------------------------------------------------------------
// Configuration and reset policy
// ---------------------------------------------------------------------------

void TestUnconfigured() {
    ChartEventCursor cursor;
    CHECK(!cursor.IsConfigured());
    CHECK(!cursor.IsStarted());
    CHECK(cursor.EventCount() == 0);
    CHECK(cursor.PendingCount() == 0);
    CHECK(cursor.ActiveGeneration() == 0);
    CHECK(cursor.LastResetGeneration() == 0);
    CHECK(!cursor.CompletedThrough());
    CHECK(!cursor.LastObserved());
    CHECK(cursor.NextEvent() == nullptr);
    CHECK(cursor.Events().empty());

    ChartEvent destination[2] = {};
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::not_configured);
    CHECK_CURSOR_STATUS(
        cursor.Drain(SongTime{0.0, 1}, std::span<ChartEvent>(destination)).status,
        ChartCursorStatus::not_configured);
    CHECK(!cursor.IsConfigured());
    CHECK(!cursor.IsStarted());
}

void TestConfigureValidation() {
    ChartEventCursor cursor;

    const std::vector<ChartEvent> duplicate_ids = {At(7, 0.0), At(7, 1000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(duplicate_ids), ChartTableStatus::duplicate_id);
    CHECK(!cursor.IsConfigured());
    CHECK(cursor.EventCount() == 0);

    const std::vector<ChartEvent> nan_time = {
        At(1, 0.0), At(2, std::numeric_limits<double>::quiet_NaN())};
    CHECK_TABLE_STATUS(cursor.Configure(nan_time), ChartTableStatus::non_finite_time);
    CHECK(!cursor.IsConfigured());

    const std::vector<ChartEvent> infinite_time = {
        At(1, 0.0), At(2, std::numeric_limits<double>::infinity())};
    CHECK_TABLE_STATUS(cursor.Configure(infinite_time), ChartTableStatus::non_finite_time);
    CHECK(!cursor.IsConfigured());

    // Unsorted input, equal times out of source order on purpose.
    const std::vector<ChartEvent> valid = {At(5, 3000.0, 5, 50), At(1, 1000.0),
                                           At(3, 2000.0, 3, 30), At(2, 2000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(valid), ChartTableStatus::accepted);
    CHECK(cursor.IsConfigured());
    CHECK(!cursor.IsStarted());
    CHECK(cursor.EventCount() == 4);
    CHECK(cursor.ActiveGeneration() == 0);
    CHECK(cursor.LastResetGeneration() == 0);
    CHECK(!cursor.CompletedThrough());
    CHECK(cursor.NextEvent() == nullptr);

    const std::span<const ChartEvent> table = cursor.Events();
    CHECK(table.size() == 4);
    CHECK(table[0].id == 1);
    CHECK(table[1].id == 3);
    CHECK(table[2].id == 2);
    CHECK(table[3].id == 5);
    CHECK(table[1].kind == 3);
    CHECK(table[1].payload == 30);
    CHECK(table[3].kind == 5);
    CHECK(table[3].payload == 50);

    ChartEventCursor empty_cursor;
    CHECK_TABLE_STATUS(empty_cursor.Configure(std::span<const ChartEvent>()),
                       ChartTableStatus::accepted);
    CHECK(empty_cursor.IsConfigured());
    CHECK(empty_cursor.EventCount() == 0);
}

void TestResetGenerationPolicy() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(1, 0.0), At(2, 1000.0),
                                           At(3, 1000.0), At(4, 2000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);

    CursorSnapshot before = Snap(cursor);
    CHECK_CURSOR_STATUS(cursor.Reset(0, 0.0), ChartCursorStatus::generation_zero);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);
    CHECK(cursor.IsStarted());
    CHECK(cursor.ActiveGeneration() == 1);
    CHECK(cursor.LastResetGeneration() == 1);
    CHECK(cursor.PendingCount() == 4);
    CHECK(cursor.NextEvent() != nullptr);
    CHECK(cursor.NextEvent()->id == 1);
    CHECK(cursor.LastObserved()->microseconds == 0.0);

    before = Snap(cursor);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 500.0), ChartCursorStatus::generation_not_new);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK_CURSOR_STATUS(cursor.Reset(0, 500.0), ChartCursorStatus::generation_zero);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    // A rejected reset must not consume the next generation number.
    CHECK_CURSOR_STATUS(cursor.Reset(2, std::numeric_limits<double>::quiet_NaN()),
                        ChartCursorStatus::non_finite_time);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK_CURSOR_STATUS(cursor.Reset(2, 0.0), ChartCursorStatus::ok);

    // Explicit seek forward with a new generation.
    CHECK_CURSOR_STATUS(cursor.Reset(3, 1000.0), ChartCursorStatus::ok);
    CHECK(cursor.ActiveGeneration() == 3);
    CHECK(cursor.LastResetGeneration() == 3);
    CHECK(cursor.PendingCount() == 3);
    CHECK(cursor.NextEvent()->id == 2);
    CHECK_CURSOR_STATUS(cursor.Reset(2, 0.0), ChartCursorStatus::generation_not_new);
    CHECK_CURSOR_STATUS(cursor.Reset(3, 0.0), ChartCursorStatus::generation_not_new);

    // Explicit seek backward is allowed only through Reset with a newer epoch.
    CHECK_CURSOR_STATUS(cursor.Reset(4, 0.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 4);
    CHECK(cursor.NextEvent()->id == 1);

    // Start exactly at an event time includes that event.
    CHECK_CURSOR_STATUS(cursor.Reset(5, 1000.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 3);
    CHECK(cursor.NextEvent()->id == 2);
}

void TestStartInclusiveAndPreroll() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {
        At(10, -2000000.0), At(20, -1000000.0), At(21, -1000000.0),
        At(30, 0.0),        At(40, 500000.0),   At(50, 1500000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);

    CHECK_CURSOR_STATUS(cursor.Reset(1, -1000000.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 5);
    CHECK(cursor.NextEvent()->id == 20);

    ChartEvent destination[10];
    ChartEventDrain drain =
        cursor.Drain(SongTime{-1000000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 2);
    CHECK(drain.drained_through_now);
    CHECK(destination[0].id == 20);
    CHECK(destination[1].id == 21);
    CHECK(cursor.CompletedThrough()->microseconds == -1000000.0);
    CHECK(cursor.PendingCount() == 3);

    CHECK_CURSOR_STATUS(cursor.Reset(2, 0.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 3);
    drain = cursor.Drain(SongTime{0.0, 2}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 30);
    CHECK(cursor.CompletedThrough()->microseconds == 0.0);

    CHECK_CURSOR_STATUS(cursor.Reset(3, 1000000.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 1);
    drain = cursor.Drain(SongTime{1000000.0, 3}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 1000000.0);
    drain = cursor.Drain(SongTime{1500000.0, 3}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 50);

    CHECK_CURSOR_STATUS(cursor.Reset(4, 2000000.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 0);
    CHECK(cursor.NextEvent() == nullptr);
    drain = cursor.Drain(SongTime{2000000.0, 4}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 2000000.0);

    // Large negative origin permitted; only finiteness is validated.
    CHECK_CURSOR_STATUS(cursor.Reset(5, -1000000000.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 6);
    drain = cursor.Drain(SongTime{2000000.0, 5}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 6);
    const std::vector<std::uint64_t> expected = {10, 20, 21, 30, 40, 50};
    std::vector<std::uint64_t> actual;
    for (std::size_t i = 0; i < drain.written; ++i) actual.push_back(destination[i].id);
    CHECK_IDS(actual, expected);
}

// ---------------------------------------------------------------------------
// Stable ordering and repeated renders
// ---------------------------------------------------------------------------

void TestStableEqualTimeOrdering() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(50, 1000.0, 7, 99), At(10, 500.0),
                                           At(30, 1000.0), At(20, 1000.0),
                                           At(40, 2000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);

    std::vector<std::uint64_t> table_ids;
    for (const ChartEvent& event : cursor.Events()) table_ids.push_back(event.id);
    const std::vector<std::uint64_t> table_expected = {10, 50, 30, 20, 40};
    CHECK_IDS(table_ids, table_expected);

    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);
    ChartEvent destination[8];
    const ChartEventDrain drain =
        cursor.Drain(SongTime{2000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 5);
    CHECK(drain.drained_through_now);
    CHECK(destination[1].kind == 7);
    CHECK(destination[1].payload == 99);

    std::vector<std::uint64_t> drained;
    for (std::size_t i = 0; i < drain.written; ++i) drained.push_back(destination[i].id);
    CHECK_IDS(drained, table_expected);
}

void TestRepeatedRenderTimes() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(1, 1000.0), At(2, 1000.0), At(3, 2000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);

    ChartEvent destination[4];
    ChartEventDrain drain =
        cursor.Drain(SongTime{1000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 2);
    CHECK(drain.drained_through_now);
    CHECK(cursor.PendingCount() == 1);
    CHECK(cursor.CompletedThrough()->microseconds == 1000.0);

    drain = cursor.Drain(SongTime{1000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 1000.0);

    drain = cursor.Drain(SongTime{1500.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 1500.0);

    drain = cursor.Drain(SongTime{2000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 3);
    drain = cursor.Drain(SongTime{2000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    drain = cursor.Drain(SongTime{2010.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(cursor.CompletedThrough()->microseconds == 2010.0);
    CHECK(cursor.LastObserved()->microseconds == 2010.0);

    // Same render time continues a budget-limited drain.
    ChartEventCursor tied;
    const std::vector<ChartEvent> tied_chart = {At(1, 1000.0), At(2, 1000.0),
                                                At(3, 1000.0)};
    CHECK_TABLE_STATUS(tied.Configure(tied_chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(tied.Reset(2, 0.0), ChartCursorStatus::ok);
    ChartEvent one[1];
    drain = tied.Drain(SongTime{1000.0, 2}, std::span<ChartEvent>(one));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(!drain.drained_through_now);
    CHECK(!tied.CompletedThrough());
    drain = tied.Drain(SongTime{1000.0, 2}, std::span<ChartEvent>(one));
    CHECK(drain.ok());
    CHECK(one[0].id == 2);
    CHECK(!drain.drained_through_now);
    drain = tied.Drain(SongTime{1000.0, 2}, std::span<ChartEvent>(one));
    CHECK(drain.ok());
    CHECK(one[0].id == 3);
    CHECK(drain.drained_through_now);
    CHECK(tied.CompletedThrough()->microseconds == 1000.0);
    CHECK(tied.PendingCount() == 0);
}

// ---------------------------------------------------------------------------
// Capacity, backlog, and watermark
// ---------------------------------------------------------------------------

void TestZeroCapacity() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(1, 0.0), At(2, 1000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);

    ChartEventDrain drain =
        cursor.Drain(SongTime{1000.0, 1}, std::span<ChartEvent>());
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::empty_destination);
    CHECK(drain.written == 0);
    CHECK(!drain.drained_through_now);
    CHECK(cursor.PendingCount() == 2);
    CHECK(!cursor.CompletedThrough());
    CHECK(cursor.LastObserved()->microseconds == 0.0);

    ChartEvent destination[1];
    drain = cursor.Drain(SongTime{1000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 1);
    CHECK(cursor.PendingCount() == 1);

    // Empty destination with a backlog still must not advance anything.
    drain = cursor.Drain(SongTime{2000.0, 1}, std::span<ChartEvent>());
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::empty_destination);
    CHECK(!drain.drained_through_now);
    CHECK(cursor.LastObserved()->microseconds == 1000.0);
    CHECK(cursor.PendingCount() == 1);
    CHECK(!cursor.CompletedThrough());

    drain = cursor.Drain(SongTime{2000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    drain = cursor.Drain(SongTime{3000.0, 1}, std::span<ChartEvent>());
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::empty_destination);
    CHECK(!drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 2000.0);
    CHECK(cursor.PendingCount() == 0);
}

void TestSmallBudgetsAndPartialDrains() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(1, 100.0), At(2, 200.0), At(3, 200.0),
                                           At(4, 300.0), At(5, 400.0), At(6, 500.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);

    ChartEvent destination[2];
    ChartEventDrain drain =
        cursor.Drain(SongTime{500.0, 1}, std::span<ChartEvent>(destination, 2));
    CHECK(drain.ok());
    CHECK(drain.written == 2);
    CHECK(destination[0].id == 1);
    CHECK(destination[1].id == 2);
    CHECK(!drain.drained_through_now);
    CHECK(!cursor.CompletedThrough());
    CHECK(cursor.PendingCount() == 4);

    drain = cursor.Drain(SongTime{500.0, 1}, std::span<ChartEvent>(destination, 1));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 3);
    CHECK(!drain.drained_through_now);
    CHECK(!cursor.CompletedThrough());

    drain = cursor.Drain(SongTime{600.0, 1}, std::span<ChartEvent>(destination, 2));
    CHECK(drain.ok());
    CHECK(drain.written == 2);
    CHECK(destination[0].id == 4);
    CHECK(destination[1].id == 5);
    CHECK(!drain.drained_through_now);
    CHECK(cursor.PendingCount() == 1);

    drain = cursor.Drain(SongTime{600.0, 1}, std::span<ChartEvent>(destination, 2));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 6);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 600.0);
    CHECK(cursor.PendingCount() == 0);

    // A later requested now with an older backlog: deliver the backlog first.
    ChartEventCursor backlog;
    const std::vector<ChartEvent> backlog_chart = {At(7, 1000.0), At(8, 2000.0),
                                                    At(9, 3000.0), At(10, 4000.0)};
    CHECK_TABLE_STATUS(backlog.Configure(backlog_chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(backlog.Reset(2, 0.0), ChartCursorStatus::ok);
    ChartEvent one[1];
    drain = backlog.Drain(SongTime{4000.0, 2}, std::span<ChartEvent>(one));
    CHECK(drain.ok());
    CHECK(one[0].id == 7);
    CHECK(!drain.drained_through_now);
    drain = backlog.Drain(SongTime{9000.0, 2}, std::span<ChartEvent>(one));
    CHECK(drain.ok());
    CHECK(one[0].id == 8);
    CHECK(!drain.drained_through_now);
    CHECK(!backlog.CompletedThrough());
    drain = backlog.Drain(SongTime{9000.0, 2}, std::span<ChartEvent>(one, 1));
    CHECK(drain.ok());
    CHECK(one[0].id == 9);
    drain = backlog.Drain(SongTime{9000.0, 2}, std::span<ChartEvent>(one, 1));
    CHECK(drain.ok());
    CHECK(one[0].id == 10);
    CHECK(drain.drained_through_now);
    CHECK(backlog.CompletedThrough()->microseconds == 9000.0);
}

void TestBackwardAndStaleTimestamps() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(1, 0.0), At(2, 1000.0), At(3, 2000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);

    ChartEvent destination[2] = {At(9001, 0.0), At(9002, 0.0)};
    ChartEventDrain drain = cursor.Drain(SongTime{1000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 2);
    CHECK(cursor.CompletedThrough()->microseconds == 1000.0);

    destination[0] = At(9001, 0.0);
    destination[1] = At(9002, 0.0);
    const CursorSnapshot before = Snap(cursor);

    drain = cursor.Drain(SongTime{999.0, 1}, std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::backward_time);
    CHECK(drain.written == 0);
    CHECK(!drain.drained_through_now);
    CHECK(destination[0].id == 9001);
    CHECK(destination[1].id == 9002);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    drain = cursor.Drain(SongTime{std::numeric_limits<double>::quiet_NaN(), 1},
                         std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::non_finite_time);
    CHECK(destination[1].id == 9002);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    drain = cursor.Drain(SongTime{-std::numeric_limits<double>::infinity(), 1},
                         std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::non_finite_time);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    drain = cursor.Drain(SongTime{1000.0, 0}, std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::stale_generation);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    drain = cursor.Drain(SongTime{1000.0, 2}, std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::stale_generation);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    // Equal time is not backward; it may finish a budget-limited drain.
    drain = cursor.Drain(SongTime{1000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 1000.0);

    drain = cursor.Drain(SongTime{2000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 3);

    // A new generation starts a new segment; old-generation drains stay stale.
    CHECK_CURSOR_STATUS(cursor.Reset(2, 500.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 2);
    drain = cursor.Drain(SongTime{500.0, 1}, std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::stale_generation);
    drain = cursor.Drain(SongTime{500.0, 2}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(drain.drained_through_now);
    drain = cursor.Drain(SongTime{1000.0, 2}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 2);
}

void TestConfigureFailurePreservesState() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(1, 1000.0), At(2, 2000.0), At(3, 3000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(4, 0.0), ChartCursorStatus::ok);

    ChartEvent destination[2];
    ChartEventDrain drain =
        cursor.Drain(SongTime{2000.0, 4}, std::span<ChartEvent>(destination, 1));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 1);
    CHECK(!cursor.CompletedThrough());

    const CursorSnapshot before = Snap(cursor);

    const std::vector<ChartEvent> duplicates = {At(9, 0.0), At(9, 10.0)};
    CHECK_TABLE_STATUS(cursor.Configure(duplicates), ChartTableStatus::duplicate_id);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    const std::vector<ChartEvent> bad_time = {At(9, 0.0),
                                              At(10, std::numeric_limits<double>::infinity())};
    CHECK_TABLE_STATUS(cursor.Configure(bad_time), ChartTableStatus::non_finite_time);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    // Playback continues on the old table after rejected configuration.
    drain = cursor.Drain(SongTime{2000.0, 4}, std::span<ChartEvent>(destination, 1));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 2);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 2000.0);
    CHECK(cursor.PendingCount() == 1);

    // Accepted reconfiguration invalidates playback but keeps the generation
    // floor, so the epoch used by the previous segment cannot be reused.
    const std::vector<ChartEvent> replacement = {At(10, 500.0)};
    CHECK_TABLE_STATUS(cursor.Configure(replacement), ChartTableStatus::accepted);
    CHECK(cursor.IsConfigured());
    CHECK(!cursor.IsStarted());
    CHECK(cursor.EventCount() == 1);
    CHECK(cursor.PendingCount() == 0);
    CHECK(cursor.ActiveGeneration() == 0);
    CHECK(cursor.LastResetGeneration() == 4);
    CHECK(!cursor.CompletedThrough());
    CHECK(!cursor.LastObserved());
    CHECK(cursor.NextEvent() == nullptr);
    drain = cursor.Drain(SongTime{500.0, 4}, std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::not_started);
    CHECK(cursor.PendingCount() == 0);

    const CursorSnapshot reconfig = Snap(cursor);
    CHECK_CURSOR_STATUS(cursor.Reset(4, 0.0), ChartCursorStatus::generation_not_new);
    CHECK_SAME_SNAPSHOT(Snap(cursor), reconfig);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::generation_not_new);
    CHECK_SAME_SNAPSHOT(Snap(cursor), reconfig);

    CHECK_CURSOR_STATUS(cursor.Reset(5, 0.0), ChartCursorStatus::ok);
    drain = cursor.Drain(SongTime{500.0, 5}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 10);
}

void TestGenerationFloorAcrossConfigure() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> first = {At(1, 0.0), At(2, 1000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(first), ChartTableStatus::accepted);
    CHECK(cursor.LastResetGeneration() == 0);

    // The first successful Reset may use any nonzero generation.
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);
    CHECK(cursor.LastResetGeneration() == 1);

    const std::vector<ChartEvent> second = {At(10, 500.0)};
    CHECK_TABLE_STATUS(cursor.Configure(second), ChartTableStatus::accepted);
    CHECK(cursor.LastResetGeneration() == 1);
    CursorSnapshot before = Snap(cursor);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::generation_not_new);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK_CURSOR_STATUS(cursor.Reset(0, 0.0), ChartCursorStatus::generation_zero);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK_CURSOR_STATUS(cursor.Reset(2, 0.0), ChartCursorStatus::ok);
    CHECK(cursor.ActiveGeneration() == 2);

    // Rejected and throwing Configure() calls leave the floor unchanged.
    const std::vector<ChartEvent> duplicate = {At(3, 0.0), At(3, 1.0)};
    before = Snap(cursor);
    CHECK_TABLE_STATUS(cursor.Configure(duplicate), ChartTableStatus::duplicate_id);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK(cursor.LastResetGeneration() == 2);

    bool threw = false;
    {
        g_fail_after_allocation.store(0, std::memory_order_relaxed);
        try {
            (void)cursor.Configure(second);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        g_fail_after_allocation.store(-1, std::memory_order_relaxed);
    }
    CHECK(threw);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK(cursor.LastResetGeneration() == 2);
    CHECK_CURSOR_STATUS(cursor.Reset(2, 0.0), ChartCursorStatus::generation_not_new);
    CHECK_CURSOR_STATUS(cursor.Reset(3, 0.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 1);
}

void TestGenerationFloorExhaustion() {
    constexpr std::uint64_t kMax = std::numeric_limits<std::uint64_t>::max();
    const std::vector<ChartEvent> chart = {At(1, 0.0)};

    ChartEventCursor cursor;
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(kMax, 0.0), ChartCursorStatus::ok);
    CHECK(cursor.LastResetGeneration() == kMax);
    CHECK(cursor.ActiveGeneration() == kMax);

    // The floor survives a new chart, so no epoch is left above it.
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    const CursorSnapshot before = Snap(cursor);
    CHECK_CURSOR_STATUS(cursor.Reset(kMax, 0.0), ChartCursorStatus::generation_not_new);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK_CURSOR_STATUS(cursor.Reset(kMax - 1, 0.0), ChartCursorStatus::generation_not_new);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::generation_not_new);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK(cursor.LastResetGeneration() == kMax);
    CHECK(!cursor.IsStarted());

    ChartEvent destination[1];
    const ChartEventDrain drain =
        cursor.Drain(SongTime{0.0, kMax}, std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::not_started);
    CHECK(drain.written == 0);

    // Exhaustion is per instance; a fresh cursor starts a new lifetime.
    ChartEventCursor fresh;
    CHECK_TABLE_STATUS(fresh.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(fresh.Reset(kMax, 0.0), ChartCursorStatus::ok);
}

void TestConfigureBadAllocPreservesState() {
    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(1, 1000.0), At(2, 2000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(3, 0.0), ChartCursorStatus::ok);

    const std::vector<ChartEvent> replacement = {At(20, 10.0), At(21, 20.0),
                                                 At(22, 30.0)};
    const CursorSnapshot before = Snap(cursor);

    bool threw = false;
    {
        g_fail_after_allocation.store(0, std::memory_order_relaxed);
        try {
            (void)cursor.Configure(replacement);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        g_fail_after_allocation.store(-1, std::memory_order_relaxed);
    }
    CHECK(threw);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);

    threw = false;
    {
        g_fail_after_allocation.store(1, std::memory_order_relaxed);
        try {
            (void)cursor.Configure(replacement);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        g_fail_after_allocation.store(-1, std::memory_order_relaxed);
    }
    CHECK(threw);
    CHECK_SAME_SNAPSHOT(Snap(cursor), before);
    CHECK(cursor.LastResetGeneration() == 3);

    ChartEvent destination[2];
    const ChartEventDrain drain =
        cursor.Drain(SongTime{2000.0, 3}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 2);
    CHECK(cursor.PendingCount() == 0);
}

// ---------------------------------------------------------------------------
// Empty chart, end of chart, watermark
// ---------------------------------------------------------------------------

void TestNoEventsAndEndOfChart() {
    ChartEventCursor empty_cursor;
    CHECK_TABLE_STATUS(empty_cursor.Configure(std::span<const ChartEvent>()),
                       ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(empty_cursor.Reset(1, -500000.0), ChartCursorStatus::ok);
    CHECK(empty_cursor.PendingCount() == 0);
    CHECK(empty_cursor.NextEvent() == nullptr);

    ChartEvent destination[2];
    ChartEventDrain drain =
        empty_cursor.Drain(SongTime{-500000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(drain.drained_through_now);
    CHECK(empty_cursor.CompletedThrough()->microseconds == -500000.0);

    drain = empty_cursor.Drain(SongTime{-500000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(empty_cursor.CompletedThrough()->microseconds == -500000.0);

    drain = empty_cursor.Drain(SongTime{1000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(empty_cursor.CompletedThrough()->microseconds == 1000.0);

    drain = empty_cursor.Drain(SongTime{999.0, 1}, std::span<ChartEvent>(destination));
    CHECK_CURSOR_STATUS(drain.status, ChartCursorStatus::backward_time);
    CHECK(empty_cursor.CompletedThrough()->microseconds == 1000.0);

    ChartEventCursor cursor;
    const std::vector<ChartEvent> chart = {At(1, 0.0), At(2, 1000.0)};
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);

    drain = cursor.Drain(SongTime{5000.0, 1}, std::span<ChartEvent>(destination, 1));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 1);
    CHECK(!drain.drained_through_now);
    CHECK(!cursor.CompletedThrough());

    drain = cursor.Drain(SongTime{5000.0, 1}, std::span<ChartEvent>(destination, 1));
    CHECK(drain.ok());
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 2);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 5000.0);
    CHECK(cursor.PendingCount() == 0);

    drain = cursor.Drain(SongTime{6000.0, 1}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(cursor.CompletedThrough()->microseconds == 6000.0);

    CHECK_CURSOR_STATUS(cursor.Reset(2, 1000000000.0), ChartCursorStatus::ok);
    CHECK(cursor.PendingCount() == 0);
    drain = cursor.Drain(SongTime{1000000000.0, 2}, std::span<ChartEvent>(destination));
    CHECK(drain.ok());
    CHECK(drain.written == 0);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 1000000000.0);
}

void TestWatermarkNeverAheadOfRetainedEvents() {
    ChartEventCursor cursor;
    std::vector<ChartEvent> chart;
    for (std::uint64_t i = 1; i <= 40; ++i) {
        chart.push_back(At(i, static_cast<double>(i) * 1000.0));
    }
    chart.push_back(At(50, 300000.0));
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);

    ChartEvent destination[64];
    std::vector<std::uint64_t> delivered;

    auto drain_once = [&](double now, std::size_t capacity) {
        const ChartEventDrain drain =
            cursor.Drain(SongTime{now, 1}, std::span<ChartEvent>(destination, capacity));
        CHECK(drain.ok());
        for (std::size_t i = 0; i < drain.written; ++i) {
            delivered.push_back(destination[i].id);
        }
        if (const auto watermark = cursor.CompletedThrough()) {
            const ChartEvent* next = cursor.NextEvent();
            CHECK(next == nullptr || watermark->microseconds < next->song_us);
        }
        return drain;
    };

    ChartEventDrain drain = drain_once(40000.0, 7);
    CHECK(drain.written == 7);
    CHECK(!drain.drained_through_now);
    CHECK(!cursor.CompletedThrough());
    CHECK(cursor.PendingCount() == 34);
    CHECK(cursor.LastObserved()->microseconds == 40000.0);

    // Observed time moves far ahead while older backlog is still retained.
    drain = drain_once(100000.0, 7);
    CHECK(drain.written == 7);
    CHECK(!drain.drained_through_now);
    CHECK(!cursor.CompletedThrough());
    CHECK(cursor.PendingCount() == 27);

    drain = drain_once(200000.0, 64);
    CHECK(drain.written == 26);
    CHECK(drain.drained_through_now);
    CHECK(cursor.CompletedThrough()->microseconds == 200000.0);
    CHECK(cursor.PendingCount() == 1);
    CHECK(cursor.NextEvent()->id == 50);
    CHECK(cursor.NextEvent()->song_us == 300000.0);
    CHECK(cursor.CompletedThrough()->microseconds < cursor.NextEvent()->song_us);

    drain = drain_once(300000.0, 64);
    CHECK(drain.written == 1);
    CHECK(destination[0].id == 50);
    CHECK(cursor.CompletedThrough()->microseconds == 300000.0);
    CHECK(cursor.PendingCount() == 0);

    std::vector<std::uint64_t> expected;
    for (std::uint64_t i = 1; i <= 40; ++i) expected.push_back(i);
    expected.push_back(50);
    CHECK_IDS(delivered, expected);
}

// ---------------------------------------------------------------------------
// Render-schedule invariance
// ---------------------------------------------------------------------------

std::vector<ChartEvent> BuildDenseChart() {
    std::vector<ChartEvent> chart;
    std::uint64_t id = 1;
    const auto add = [&chart, &id](double song_us) {
        chart.push_back(At(id++, song_us));
    };

    add(-2000000.0);
    add(-1500000.0);
    add(-1000000.0);
    add(-1000000.0);

    for (int i = 0; i < 300; ++i) {
        const double time = static_cast<double>(i) * 300000.0;
        add(time);
        if (i % 7 == 0) {
            add(time);
            add(time);
        }
        if (i % 31 == 0) {
            add(time + 1.0);
        }
    }

    // Dense burst crossed by the injected 1.53 s presentation stall.
    for (int i = 0; i < 40; ++i) {
        add(60000000.0 + static_cast<double>(i) * 250.0);
    }

    add(118000000.0);
    add(119500000.0);
    return chart;
}

struct ScheduleRun final {
    bool configured_ok = false;
    bool invariants_ok = true;
    bool order_matches = true;
    bool final_watermark_ok = false;
    bool stall_observed = false;
    std::size_t max_pending = 0;
    std::size_t calls = 0;
    std::vector<std::uint64_t> ids;
};

ScheduleRun RunSchedule(const std::vector<ChartEvent>& chart,
                        const std::vector<std::uint64_t>& expected,
                        unsigned rate_hz, std::size_t capacity, std::uint64_t seed) {
    ScheduleRun run;
    ChartEventCursor cursor;
    run.configured_ok = cursor.Configure(chart) == ChartTableStatus::accepted;
    if (!run.configured_ok) return run;

    constexpr std::uint64_t kGeneration = 11;
    constexpr double kStart = -2000000.0;
    constexpr double kStallAt = 60000000.0;
    constexpr double kStallLength = 1530000.0;
    constexpr double kLastEvent = 119500000.0;
    constexpr double kFinalNow = 130000000.0;

    if (cursor.Reset(kGeneration, kStart) != ChartCursorStatus::ok) {
        run.invariants_ok = false;
        return run;
    }

    std::vector<ChartEvent> buffer(capacity);
    std::span<ChartEvent> destination(buffer);

    std::uint64_t rng = seed;
    const auto next_unit = [&rng]() {
        rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(rng >> 11) * (1.0 / 9007199254740992.0);
    };

    double now = kStart;
    bool stalled = false;
    std::optional<double> last_watermark;

    const auto consume = [&]() {
        const ChartEventDrain drain =
            cursor.Drain(SongTime{now, kGeneration}, destination);
        ++run.calls;
        if (!drain.ok()) {
            run.invariants_ok = false;
            return false;
        }
        for (std::size_t i = 0; i < drain.written; ++i) {
            const std::uint64_t id = buffer[i].id;
            if (run.ids.size() >= expected.size() ||
                expected[run.ids.size()] != id) {
                run.order_matches = false;
            }
            run.ids.push_back(id);
        }
        if (const auto watermark = cursor.CompletedThrough()) {
            if (last_watermark && watermark->microseconds < *last_watermark) {
                run.invariants_ok = false;
            }
            last_watermark = watermark->microseconds;
            const ChartEvent* next = cursor.NextEvent();
            if (next != nullptr && !(watermark->microseconds < next->song_us)) {
                run.invariants_ok = false;
            }
        }
        run.max_pending = std::max(run.max_pending, cursor.PendingCount());
        return true;
    };

    const double base_dt = 1000000.0 / static_cast<double>(rate_hz);
    std::uint64_t frame = 0;
    while (now < kLastEvent + 1000000.0 && frame < 2000000) {
        // Positive, deterministic jitter around the nominal frame period.
        now += base_dt * (0.65 + 0.7 * next_unit());
        if (!stalled && now >= kStallAt) {
            now += kStallLength;
            stalled = true;
        }
        if (!consume()) break;
        if (frame % 17 == 16 && !consume()) break; // repeated render of same time
        ++frame;
    }

    now = kFinalNow;
    run.stall_observed = stalled;
    for (int guard = 0; guard < 20000 && cursor.PendingCount() > 0; ++guard) {
        if (!consume()) break;
    }
    if (run.invariants_ok && !consume()) {
        run.invariants_ok = false;
    }

    if (run.ids.size() != expected.size()) run.order_matches = false;
    const auto watermark = cursor.CompletedThrough();
    run.final_watermark_ok =
        watermark.has_value() && cursor.PendingCount() == 0 &&
        watermark->microseconds >= kFinalNow && watermark->generation == kGeneration;
    return run;
}

void TestRenderScheduleInvariance() {
    const std::vector<ChartEvent> chart = BuildDenseChart();

    // The source table is deliberately not in time order.
    std::vector<ChartEvent> shuffled = chart;
    std::mt19937 shuffle_engine(20261029u);
    std::shuffle(shuffled.begin(), shuffled.end(), shuffle_engine);

    // Delivery order is Configure()'s stable sort of that shuffled source, so
    // equal-time events keep their shuffled relative order.
    std::vector<ChartEvent> sorted = shuffled;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const ChartEvent& lhs, const ChartEvent& rhs) {
                         return lhs.song_us < rhs.song_us;
                     });
    std::vector<std::uint64_t> expected;
    expected.reserve(sorted.size());
    for (const ChartEvent& event : sorted) expected.push_back(event.id);

    const unsigned rates[] = {60, 90, 120, 144, 165, 240, 360};
    const std::size_t capacities[] = {8, 1, 3, 5, 7, 2, 6};
    std::vector<std::vector<std::uint64_t>> all_ids;

    for (std::size_t i = 0; i < std::size(rates); ++i) {
        const std::uint64_t seed =
            0x9E3779B97F4A7C15ULL * static_cast<std::uint64_t>(i + 1) + 12345ULL;
        const ScheduleRun run =
            RunSchedule(shuffled, expected, rates[i], capacities[i], seed);
        CHECK(run.configured_ok);
        CHECK(run.invariants_ok);
        CHECK(run.order_matches);
        CHECK(run.final_watermark_ok);
        CHECK(run.stall_observed);
        CHECK(run.max_pending > 0);
        CHECK(run.ids.size() == expected.size());
        CHECK(run.calls > 0);
        CHECK_IDS(run.ids, expected);
        all_ids.push_back(run.ids);
    }

    for (std::size_t i = 1; i < all_ids.size(); ++i) {
        CHECK_IDS(all_ids[i], all_ids[0]);
    }
}

// ---------------------------------------------------------------------------
// Hot path allocation freedom
// ---------------------------------------------------------------------------

void TestHotPathDoesNotAllocate() {
    std::vector<ChartEvent> chart;
    for (std::uint64_t i = 1; i <= 64; ++i) {
        chart.push_back(At(i, static_cast<double>(i) * 1000.0));
    }

    ChartEventCursor cursor;
    const std::size_t before_configure =
        g_allocation_count.load(std::memory_order_relaxed);
    CHECK_TABLE_STATUS(cursor.Configure(chart), ChartTableStatus::accepted);
    // Sanity check that the replacement allocator observes the configured table.
    CHECK(g_allocation_count.load(std::memory_order_relaxed) > before_configure);

    ChartEvent destination[8];
    const std::size_t allocations_before =
        g_allocation_count.load(std::memory_order_relaxed);

    CHECK_CURSOR_STATUS(cursor.Reset(1, 0.0), ChartCursorStatus::ok);
    std::size_t written_total = 0;
    for (int round = 0; round < 200; ++round) {
        const double now = static_cast<double>(round) * 500.0;
        const ChartEventDrain drain =
            cursor.Drain(SongTime{now, 1}, std::span<ChartEvent>(destination));
        written_total += drain.written;
        const auto watermark = cursor.CompletedThrough();
        const auto observed = cursor.LastObserved();
        const ChartEvent* next = cursor.NextEvent();
        (void)watermark;
        (void)observed;
        (void)next;

        const ChartEventDrain backward =
            cursor.Drain(SongTime{-1.0, 1}, std::span<ChartEvent>(destination));
        (void)backward;
        const ChartEventDrain empty =
            cursor.Drain(SongTime{now, 1}, std::span<ChartEvent>());
        (void)empty;
        const ChartEventDrain stale =
            cursor.Drain(SongTime{now, 2}, std::span<ChartEvent>(destination));
        (void)stale;
    }
    const std::size_t allocations_after =
        g_allocation_count.load(std::memory_order_relaxed);

    CHECK(allocations_after == allocations_before);
    CHECK(written_total == 64);
    CHECK(cursor.PendingCount() == 0);
}

} // namespace

int main() {
    struct TestCase final {
        const char* name;
        void (*function)();
    };

    const TestCase tests[] = {
        {"unconfigured", TestUnconfigured},
        {"configure_validation", TestConfigureValidation},
        {"reset_generation_policy", TestResetGenerationPolicy},
        {"start_inclusive_and_preroll", TestStartInclusiveAndPreroll},
        {"stable_equal_time_ordering", TestStableEqualTimeOrdering},
        {"repeated_render_times", TestRepeatedRenderTimes},
        {"zero_capacity", TestZeroCapacity},
        {"small_budgets_and_partial_drains", TestSmallBudgetsAndPartialDrains},
        {"backward_and_stale_timestamps", TestBackwardAndStaleTimestamps},
        {"configure_failure_preserves_state", TestConfigureFailurePreservesState},
        {"configure_bad_alloc_preserves_state", TestConfigureBadAllocPreservesState},
        {"generation_floor_across_configure", TestGenerationFloorAcrossConfigure},
        {"generation_floor_exhaustion", TestGenerationFloorExhaustion},
        {"no_events_and_end_of_chart", TestNoEventsAndEndOfChart},
        {"watermark_never_ahead_of_retained", TestWatermarkNeverAheadOfRetainedEvents},
        {"render_schedule_invariance", TestRenderScheduleInvariance},
        {"hot_path_does_not_allocate", TestHotPathDoesNotAllocate},
    };

    std::printf("chart_event_cursor tests: %zu cases\n", std::size(tests));
    for (const TestCase& test : tests) {
        g_test = test.name;
        const int failures_before = g_failures;
        test.function();
        std::printf("%s %s\n", g_failures == failures_before ? "PASS" : "FAIL",
                    test.name);
    }
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
