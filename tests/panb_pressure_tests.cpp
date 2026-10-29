// PANB pressure path: status-buffer decoding, 28-key averaging, threshold,
// hold counter, edges, reset and the judgement-independence claim.

#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/input_decoder.hpp" // PanbVelocity, only for the cross-check
#include "nosfix/native_input_bridge.hpp"
#include "nosfix/native_note_types.hpp"
#include "nosfix/panb_pressure.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string_view>

namespace {

int g_failures = 0;

void Check(bool condition, const char* text, int line) {
    if (!condition) {
        std::printf("FAIL line %d: %s\n", line, text);
        ++g_failures;
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

using namespace nosfix;

constexpr std::size_t kMaxSamples = 8;
constexpr std::size_t kBufferBytes =
    kPanbHeaderBytes + kPanbRecordBytes * (kMaxSamples - 1) + kPanbNibbleBytes;

struct Frame final {
    std::array<std::byte, kBufferBytes> bytes{};

    void SetSamples(std::uint8_t count) { bytes[0] = static_cast<std::byte>(count); }

    void SetKey(std::uint8_t lane, std::uint8_t level, std::uint8_t sample = 0) {
        const std::size_t offset = kPanbHeaderBytes + kPanbRecordBytes * sample + lane / 2;
        const auto current = std::to_integer<std::uint8_t>(bytes[offset]);
        // Lane 0 (Key 1) is the high nibble of the first data byte.
        bytes[offset] = (lane % 2) == 0
                            ? static_cast<std::byte>((current & 0x0Fu) | (level << 4))
                            : static_cast<std::byte>((current & 0xF0u) | (level & 0x0Fu));
    }

    std::span<const std::byte> Span() const { return {bytes.data(), kBufferBytes}; }
};

PanbConfig Verified(std::uint8_t threshold = 0) {
    PanbConfig config;
    config.threshold_source = PanbThresholdSource::RuntimeSampled;
    config.threshold = threshold;
    return config;
}

struct Fixture final {
    PanbPressureState state;
    std::array<PanbKeySample, kPanbKeyCount> samples{};
    std::array<PanbEdge, kPanbKeyCount> edges{};
    PanbChangeCounts counts{};

    PanbStatus Apply(const Frame& frame) {
        return state.Apply(frame.Span(), samples, edges, counts);
    }
};

// Collects the edges of one applied frame as a compact string so a whole poll
// sequence can be compared.
struct EdgeLog final {
    char text[512]{};
    std::size_t length = 0;
    std::size_t edges = 0;

    void Append(char kind, std::uint8_t lane, std::uint8_t level) {
        const int written = std::snprintf(text + length, sizeof(text) - length, "%c%u:%u,",
                                          kind, static_cast<unsigned>(lane),
                                          static_cast<unsigned>(level));
        if (written > 0) length += static_cast<std::size_t>(written);
        ++edges;
    }
};

void LogEdges(const Fixture& fixture, EdgeLog& log) {
    for (std::size_t i = 0; i < fixture.counts.edges; ++i) {
        const PanbEdge& edge = fixture.edges[i];
        log.Append(edge.pressed ? 'P' : 'R', edge.lane, edge.level);
    }
}

// ---------------------------------------------------------------------------
void TestUnverifiedRefusesEverything() {
    Fixture fixture;
    CHECK(fixture.state.Configure(PanbConfig{}) == PanbStatus::Ok);
    CHECK(!fixture.state.Verified());
    CHECK(fixture.state.ThresholdSource() == PanbThresholdSource::Unverified);
    Frame frame;
    frame.SetSamples(1);
    frame.SetKey(0, 14);
    CHECK(fixture.Apply(frame) == PanbStatus::UnverifiedRule);
    CHECK(fixture.counts.edges == 0);
    CHECK(fixture.state.PressedCount() == 0);
    CHECK(!fixture.state.Pressed(0));
    CHECK(fixture.samples[0].level == 0);

    PanbPressureState unconfigured;
    std::array<PanbKeySample, kPanbKeyCount> samples{};
    std::array<PanbEdge, kPanbKeyCount> edges{};
    PanbChangeCounts counts{};
    CHECK(unconfigured.Apply(frame.Span(), samples, edges, counts) ==
          PanbStatus::NotConfigured);
}

void TestNibbleGeometry() {
    CHECK(PanbDataByteForLane(0) == 0);
    CHECK(PanbDataByteForLane(1) == 0);
    CHECK(PanbDataByteForLane(2) == 1);
    CHECK(PanbDataByteForLane(27) == 13);
    CHECK(PanbLaneIsHighNibble(0));
    CHECK(!PanbLaneIsHighNibble(1));
    CHECK(PanbLaneIsHighNibble(26));
    CHECK(!PanbLaneIsHighNibble(27));

    Fixture fixture;
    CHECK(fixture.state.Configure(Verified(0)) == PanbStatus::Ok);
    Frame frame;
    frame.SetSamples(1);
    frame.SetKey(0, 14); // Key 1, high nibble of byte 3
    frame.SetKey(1, 1);  // Key 2, low nibble of byte 3
    frame.SetKey(27, 7); // Key 28, low nibble of byte 16
    CHECK(fixture.Apply(frame) == PanbStatus::Ok);
    CHECK(fixture.counts.samples == 1);
    CHECK(fixture.samples[0].level == 14);
    CHECK(fixture.samples[1].level == 1);
    CHECK(fixture.samples[27].level == 7);
    CHECK(fixture.samples[2].level == 0);
    CHECK(fixture.counts.keys_nonzero == 3);
    CHECK(fixture.counts.presses == 3);
    CHECK(fixture.counts.edges == 3);
    CHECK(fixture.edges[0].lane == 0 && fixture.edges[1].lane == 1 &&
          fixture.edges[2].lane == 27);
    CHECK(fixture.state.PressedCount() == 3);
}

void TestAveragingAndThreshold() {
    // Four samples whose nibbles sum to 10: integer division gives 2.
    Fixture fixture;
    CHECK(fixture.state.Configure(Verified(0)) == PanbStatus::Ok);
    Frame frame;
    frame.SetSamples(4);
    frame.SetKey(0, 1, 0);
    frame.SetKey(0, 2, 1);
    frame.SetKey(0, 3, 2);
    frame.SetKey(0, 4, 3);
    CHECK(fixture.Apply(frame) == PanbStatus::Ok);
    CHECK(fixture.samples[0].average == 2);
    CHECK(fixture.samples[0].level == 2);

    // Unsigned threshold boundary: level == threshold passes (cmp/jae).
    const std::uint8_t thresholds[] = {0, 1, 2, 3};
    const std::uint8_t expected[] = {2, 2, 2, 0};
    for (std::size_t i = 0; i < 4; ++i) {
        Fixture per;
        CHECK(per.state.Configure(Verified(thresholds[i])) == PanbStatus::Ok);
        CHECK(per.Apply(frame) == PanbStatus::Ok);
        CHECK(per.samples[0].level == expected[i]);
        CHECK(per.samples[0].average == 2); // the raw average is kept ungated
    }
    // A gated key still reports the raw average and a zero level.
    Fixture gated;
    CHECK(gated.state.Configure(Verified(9)) == PanbStatus::Ok);
    CHECK(gated.Apply(frame) == PanbStatus::Ok);
    CHECK(gated.samples[0].average == 2);
    CHECK(gated.samples[0].level == 0);
    CHECK(gated.counts.keys_nonzero == 1);
    CHECK(gated.counts.keys_gated == 1);
    CHECK(gated.counts.edges == 0);
}

void TestEveryLevel() {
    // Level 0..14 on one key, one sample each; 15 is refused by default.
    for (std::uint8_t level = 0; level <= kPanbMaxLevel; ++level) {
        Fixture fixture;
        CHECK(fixture.state.Configure(Verified(0)) == PanbStatus::Ok);
        Frame frame;
        frame.SetSamples(1);
        frame.SetKey(4, level);
        CHECK(fixture.Apply(frame) == PanbStatus::Ok);
        CHECK(fixture.samples[4].level == level);
        CHECK(fixture.samples[4].pressed == (level != 0));
        CHECK(fixture.state.Pressed(4) == (level != 0));
        CHECK(fixture.counts.edges == (level != 0 ? 1u : 0u));
    }
    {
        // The native parser does not clamp a 4-bit level, so 15 is accepted by
        // default; the guard is opt-in for a producer that stops at 14.
        Fixture fixture;
        CHECK(fixture.state.Configure(Verified(0)) == PanbStatus::Ok);
        Frame frame;
        frame.SetSamples(1);
        frame.SetKey(4, 15);
        CHECK(fixture.Apply(frame) == PanbStatus::Ok);
        CHECK(fixture.samples[4].level == 15);
    }
    {
        PanbConfig config = Verified(0);
        config.reject_level_15 = true;
        Fixture fixture;
        CHECK(fixture.state.Configure(config) == PanbStatus::Ok);
        Frame frame;
        frame.SetSamples(1);
        frame.SetKey(4, 15);
        CHECK(fixture.Apply(frame) == PanbStatus::LevelOutOfRange);
        CHECK(fixture.counts.edges == 0);
        CHECK(fixture.state.PressedCount() == 0);
    }
}

void TestBufferValidation() {
    Fixture fixture;
    CHECK(fixture.state.Configure(Verified(0)) == PanbStatus::Ok);

    // Declared sample count above what the buffer holds.
    Frame frame;
    frame.SetSamples(9);
    std::array<PanbKeySample, kPanbKeyCount> samples{};
    std::array<PanbEdge, kPanbKeyCount> edges{};
    PanbChangeCounts counts{};
    CHECK(fixture.state.Apply(frame.Span(), samples, edges, counts) ==
          PanbStatus::MalformedBuffer);
    CHECK(counts.edges == 0);

    // Zero samples: the native loop stores nothing at all.
    Frame empty;
    empty.SetSamples(0);
    empty.SetKey(0, 12);
    CHECK(fixture.Apply(empty) == PanbStatus::EmptyFrame);
    CHECK(fixture.counts.edges == 0);
    CHECK(fixture.state.PressedCount() == 0);

    // Output spans that cannot hold 28 keys.
    std::array<PanbKeySample, 4> small_samples{};
    std::array<PanbEdge, 4> small_edges{};
    Frame good;
    good.SetSamples(1);
    good.SetKey(0, 12);
    CHECK(fixture.state.Apply(good.Span(), small_samples, small_edges, counts) ==
          PanbStatus::InsufficientCapacity);
    CHECK(fixture.state.PressedCount() == 0);
}

void TestEdgesHeldCounterAndReset() {
    Fixture fixture;
    CHECK(fixture.state.Configure(Verified(0)) == PanbStatus::Ok);

    // Press, hold, release: one press edge, no edge while held, one release.
    Frame down;
    down.SetSamples(1);
    down.SetKey(6, 12);
    CHECK(fixture.Apply(down) == PanbStatus::Ok);
    CHECK(fixture.counts.edges == 1);
    CHECK(fixture.edges[0].pressed && fixture.edges[0].level == 12);
    CHECK(fixture.samples[6].held == 1);
    CHECK(fixture.Apply(down) == PanbStatus::Ok);
    CHECK(fixture.counts.edges == 0);
    CHECK(fixture.samples[6].held == 2);
    Frame softer = down;
    softer.SetKey(6, 5);
    CHECK(fixture.Apply(softer) == PanbStatus::Ok);
    CHECK(fixture.counts.edges == 0); // level change without an edge
    CHECK(fixture.samples[6].held == 3);
    Frame up;
    up.SetSamples(1);
    CHECK(fixture.Apply(up) == PanbStatus::Ok);
    CHECK(fixture.counts.edges == 1);
    CHECK(fixture.edges[0].pressed == false);
    CHECK(fixture.edges[0].level == 0);
    CHECK(fixture.samples[6].held == 0);
    CHECK(fixture.state.PressedCount() == 0);

    // An empty frame leaves the record (and the held counter) untouched.
    CHECK(fixture.Apply(down) == PanbStatus::Ok);
    CHECK(fixture.samples[6].held == 1);
    Frame empty;
    empty.SetSamples(0);
    CHECK(fixture.Apply(empty) == PanbStatus::EmptyFrame);
    CHECK(fixture.samples[6].held == 1);
    CHECK(fixture.state.Pressed(6));

    // Reconnect / stream reset releases every held lane exactly once.
    std::array<PanbEdge, kPanbKeyCount> edges{};
    CHECK(fixture.state.Reset(edges) == 1);
    CHECK(edges[0].lane == 6 && edges[0].pressed == false);
    CHECK(fixture.state.PressedCount() == 0);
    CHECK(fixture.state.Reset(edges) == 0);
    // A fresh frame after the reconnect is a new press edge.
    CHECK(fixture.Apply(down) == PanbStatus::Ok);
    CHECK(fixture.counts.edges == 1 && fixture.edges[0].pressed);
    CHECK(fixture.samples[6].held == 1); // the counter restarted
}

void TestChordsAndRapidRepeats() {
    Fixture fixture;
    CHECK(fixture.state.Configure(Verified(0)) == PanbStatus::Ok);
    // A three-note chord in one frame produces three press edges in lane order.
    Frame chord;
    chord.SetSamples(1);
    chord.SetKey(2, 9);
    chord.SetKey(11, 14);
    chord.SetKey(25, 4);
    CHECK(fixture.Apply(chord) == PanbStatus::Ok);
    CHECK(fixture.counts.presses == 3);
    CHECK(fixture.edges[0].lane == 2);
    CHECK(fixture.edges[1].lane == 11);
    CHECK(fixture.edges[2].lane == 25);

    // Rapid same-key repeats: eight taps alternate press and release edges.
    Fixture taps;
    CHECK(taps.state.Configure(Verified(0)) == PanbStatus::Ok);
    EdgeLog log;
    Frame on;
    on.SetSamples(1);
    on.SetKey(7, 11);
    Frame off;
    off.SetSamples(1);
    for (int i = 0; i < 8; ++i) {
        CHECK(taps.Apply(on) == PanbStatus::Ok);
        LogEdges(taps, log);
        CHECK(taps.Apply(off) == PanbStatus::Ok);
        LogEdges(taps, log);
    }
    CHECK(log.edges == 16);
    CHECK(std::string_view(log.text) ==
          "P7:11,R7:0,P7:11,R7:0,P7:11,R7:0,P7:11,R7:0,"
          "P7:11,R7:0,P7:11,R7:0,P7:11,R7:0,P7:11,R7:0,");
    CHECK(taps.state.PressedCount() == 0);
}

void TestCadenceInvarianceAndJudgementIndependence() {
    // The same level transitions polled at different rates (duplicate frames
    // between changes) produce the same edge sequence.
    auto run = [](int duplicates, EdgeLog& log, std::uint32_t& held_at_press) {
        Fixture fixture;
        (void)fixture.state.Configure(Verified(0));
        Frame down;
        down.SetSamples(1);
        down.SetKey(3, 13);
        Frame up;
        up.SetSamples(1);
        for (int i = 0; i < duplicates; ++i) {
            (void)fixture.Apply(down);
            LogEdges(fixture, log);
        }
        held_at_press = fixture.samples[3].held;
        for (int i = 0; i < duplicates; ++i) {
            (void)fixture.Apply(up);
            LogEdges(fixture, log);
        }
    };
    EdgeLog fast;
    EdgeLog slow;
    std::uint32_t held_fast = 0;
    std::uint32_t held_slow = 0;
    run(1, fast, held_fast);
    run(5, slow, held_slow);
    CHECK(std::string_view(fast.text) == "P3:13,R3:0,");
    CHECK(std::string_view(slow.text) == "P3:13,R3:0,");
    CHECK(held_fast == 1);
    CHECK(held_slow == 5);

    // The judge never sees the level: presses carrying PANB 1 and PANB 14 give
    // the same rank, scale and sound. This is the claim in panb_pressure.hpp.
    NativeTypeConfig config;
    config.windows.just_early_us = 33'334.0;
    config.windows.just_late_us = 33'334.0;
    config.windows.great_early_us = 66'667.0;
    config.windows.great_late_us = 66'667.0;
    config.windows.good_early_us = 133'334.0;
    config.windows.good_late_us = 133'334.0;
    config.windows_sampled = true;
    config.units_per_second = 60.0;

    NativeLongSpec spec;
    spec.lane = 3;
    spec.start_us = 1'000'000.0;
    spec.end_us = 2'000'000.0;

    auto judge = [&](std::uint8_t level) {
        NativeLongState state;
        (void)state.Configure(spec, config);
        (void)state.Begin(7, 0.0);
        TimedInputEvent timed;
        timed.input.lane = 3;
        timed.input.edge = InputEdge::Press;
        timed.input.velocity = static_cast<float>(level) / 15.999f;
        timed.song_time.generation = 7;
        timed.song_time.microseconds = 1'000'000.0;
        NativeNoteEvent event;
        (void)MakeNativeNoteEvent(timed, 1, 0, event);
        return state.Press(event, 1'000'000.0);
    };
    const NoteTypeResult low = judge(1);
    const NoteTypeResult high = judge(14);
    CHECK(low.count == 1 && high.count == 1);
    CHECK(low.outcomes[0].rank == high.outcomes[0].rank);
    CHECK(low.outcomes[0].scale == high.outcomes[0].scale);
    CHECK(low.outcomes[0].sound_id == high.outcomes[0].sound_id);
    CHECK(low.outcomes[0].kind == high.outcomes[0].kind);
}

void TestQuantizationMatchesDecoder() {
    // PanbLevelFromNormalized must agree with the decoder's PanbVelocity.
    for (int i = 0; i <= 1000; ++i) {
        const float normalized = static_cast<float>(i) / 1000.0f;
        CHECK(PanbLevelFromNormalized(normalized) == PanbVelocity(normalized));
    }
    CHECK(PanbLevelFromNormalized(-1.0f) == 0);
    CHECK(PanbLevelFromNormalized(0.0f) == 0);
    CHECK(PanbLevelFromNormalized(1.0f) == kPanbMaxLevel);
    CHECK(PanbLevelFromNormalized(2.0f) == kPanbMaxLevel);
    CHECK(PanbLevelFromNormalized(std::numeric_limits<float>::quiet_NaN()) == 0);
    CHECK(PanbLevelFromNormalized(1.0f / 15.999f) == 1); // derived press threshold

    const PanbEdge release = MakePanbReleaseEdge(9, 0.8f);
    CHECK(release.lane == 9);
    CHECK(release.pressed == false);
    CHECK(release.level == 0);
}

} // namespace

int main() {
    TestUnverifiedRefusesEverything();
    TestNibbleGeometry();
    TestAveragingAndThreshold();
    TestEveryLevel();
    TestBufferValidation();
    TestEdgesHeldCounterAndReset();
    TestChordsAndRapidRepeats();
    TestCadenceInvarianceAndJudgementIndependence();
    TestQuantizationMatchesDecoder();

    if (g_failures == 0) {
        std::printf("PASS panb_pressure_tests\n");
        return 0;
    }
    std::printf("FAILED panb_pressure_tests (%d checks)\n", g_failures);
    return 1;
}
