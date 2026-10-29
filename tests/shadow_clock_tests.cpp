#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/shadow_clock.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {

constexpr std::int64_t kFrequency = 10'000'000;
constexpr std::int64_t kEpoch = 7'000'000'000'000'000'000LL;

void TestDisabledAndRejected() {
    nosfix::ShadowClock clock;
    assert(!clock.Enabled());
    assert(clock.Observe(1'000'000.0, kEpoch, kFrequency) ==
           nosfix::ShadowClockStatus::Disabled);
    assert(!clock.HasPublished());

    clock.Configure(true);
    assert(clock.Enabled());
    assert(clock.Observe(std::numeric_limits<double>::quiet_NaN(), kEpoch,
                         kFrequency) == nosfix::ShadowClockStatus::Rejected);
    assert(clock.Observe(1'000'000.0, 0, kFrequency) ==
           nosfix::ShadowClockStatus::Rejected);
    assert(clock.Observe(1'000'000.0, kEpoch, 0) ==
           nosfix::ShadowClockStatus::Rejected);
    assert(!clock.HasPublished());
}

void TestEnableInvalidatesStalePublication() {
    nosfix::ShadowClock clock;
    clock.Configure(true);
    nosfix::SongClockSnapshot snapshot{};
    assert(clock.Publication()->TryRead(snapshot));
    assert(!snapshot.segment.IsValid());
    assert(snapshot.continuity == nosfix::SongClockContinuity::PossiblyRegressed);
    assert(!clock.HasPublished());
}

void TestStartAndMapping() {
    nosfix::ShadowClock clock;
    clock.Configure(true);

    const auto status = clock.Observe(1'000'000.0, kEpoch, kFrequency);
    assert(status == nosfix::ShadowClockStatus::Started);
    const auto snapshot = clock.Snapshot();
    assert(snapshot.IsValid());
    assert(snapshot.Generation() == 1);
    assert(snapshot.Rate() == 1.0);

    const auto mapped = snapshot.Map(kEpoch + 1000);
    assert(mapped);
    assert(mapped->microseconds > 1'000'099.0);
    assert(mapped->microseconds < 1'000'101.0);

    nosfix::SongClockSnapshot published{};
    assert(clock.Publication()->TryRead(published));
    assert(published.segment.Generation() == 1);
    assert(published.continuity == nosfix::SongClockContinuity::Monotonic);
    assert(published.version >= 1);
}

void TestObserveAndNegativeCorrection() {
    nosfix::ShadowClock clock;
    clock.Configure(true);
    assert(clock.Observe(1'000'000.0, kEpoch, kFrequency) ==
           nosfix::ShadowClockStatus::Started);

    const auto observed = clock.Observe(1'000'100.0, kEpoch + 1000, kFrequency);
    assert(observed == nosfix::ShadowClockStatus::Observed);
    assert(clock.Snapshot().Generation() == 1);

    // 50 us behind the prediction: accepted, but the publication must say the
    // segment is not guaranteed monotonic for an older QPC.
    const auto regressed = clock.Observe(1'000'150.0, kEpoch + 2000, kFrequency);
    assert(regressed == nosfix::ShadowClockStatus::Observed);
    nosfix::SongClockSnapshot published{};
    assert(clock.Publication()->TryRead(published));
    assert(published.continuity == nosfix::SongClockContinuity::PossiblyRegressed);
}

void TestSeekOnJumpAndBackward() {
    nosfix::ShadowClock clock;
    clock.Configure(true);
    assert(clock.Observe(1'000'000.0, kEpoch, kFrequency) ==
           nosfix::ShadowClockStatus::Started);

    // Predictions move forward; a 100 ms jump is outside the 50 ms budget.
    const auto jumped = clock.Observe(1'100'000.0, kEpoch + 1000, kFrequency);
    assert(jumped == nosfix::ShadowClockStatus::Seeked);
    assert(clock.Snapshot().Generation() == 2);

    const auto backward = clock.Observe(900'000.0, kEpoch + 2000, kFrequency);
    assert(backward == nosfix::ShadowClockStatus::Seeked);
    assert(clock.Snapshot().Generation() == 3);
}

void TestFrequencyChangeAndDisable() {
    nosfix::ShadowClock clock;
    clock.Configure(true);
    assert(clock.Observe(1'000'000.0, kEpoch, kFrequency) ==
           nosfix::ShadowClockStatus::Started);
    const auto generation = clock.Snapshot().Generation();

    const auto changed = clock.Observe(1'000'100.0, kEpoch + 1000, kFrequency * 2);
    assert(changed == nosfix::ShadowClockStatus::Started);
    assert(clock.Snapshot().Generation() == generation + 1);
    assert(clock.Snapshot().Frequency() == kFrequency * 2);

    const auto before_stop = clock.Generation();
    clock.Configure(false);
    assert(!clock.Enabled());
    const auto after_stop = clock.Generation();
    assert(after_stop >= before_stop);
    assert(clock.Observe(1'000'000.0, kEpoch, kFrequency) ==
           nosfix::ShadowClockStatus::Disabled);
}

void TestPublicationIsImmutable() {
    nosfix::ShadowClock clock;
    clock.Configure(true);
    assert(clock.Observe(1'000'000.0, kEpoch, kFrequency) ==
           nosfix::ShadowClockStatus::Started);
    nosfix::SongClockSnapshot old{};
    assert(clock.Publication()->TryRead(old));

    // A later observation must not rewrite the captured snapshot's mapping.
    assert(clock.Observe(1'050'000.0, kEpoch + 500'000, kFrequency) ==
           nosfix::ShadowClockStatus::Observed);
    const auto old_mapping = old.segment.Map(kEpoch + 250'000);
    assert(old_mapping);
    assert(old_mapping->microseconds > 1'024'999.0);
    assert(old_mapping->microseconds < 1'025'001.0);
}

} // namespace

int main() {
    TestDisabledAndRejected();
    TestEnableInvalidatesStalePublication();
    TestStartAndMapping();
    TestObserveAndNegativeCorrection();
    TestSeekOnJumpAndBackward();
    TestFrequencyChangeAndDisable();
    TestPublicationIsImmutable();
    return 0;
}
