#include "nosfix/timed_ui_timeline.hpp"

#include <algorithm>
#include <cmath>

namespace nosfix {
namespace {

constexpr std::size_t kChannelCount = static_cast<std::size_t>(UiChannel::Count);
constexpr float kMaximumValue = 1.0e7f;

bool FiniteTime(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= UiTimeline::kMaximumTimeMs;
}

bool ClampedChannel(UiChannel channel) noexcept {
    switch (channel) {
    case UiChannel::Opacity:
    case UiChannel::ColorR:
    case UiChannel::ColorG:
    case UiChannel::ColorB:
    case UiChannel::ColorA:
        return true;
    default:
        return false;
    }
}

float SmoothStep(double t) noexcept {
    const double clamped = std::clamp(t, 0.0, 1.0);
    return static_cast<float>(clamped * clamped * (3.0 - 2.0 * clamped));
}

} // namespace

const char* UiTimelineDiagnosticName(UiTimelineDiagnostic value) noexcept {
    switch (value) {
    case UiTimelineDiagnostic::None: return "none";
    case UiTimelineDiagnostic::NotConfigured: return "not-configured";
    case UiTimelineDiagnostic::NoTracks: return "no-tracks";
    case UiTimelineDiagnostic::EmptyTrack: return "empty-track";
    case UiTimelineDiagnostic::UnsortedKeys: return "unsorted-keys";
    case UiTimelineDiagnostic::InvalidKey: return "invalid-key";
    case UiTimelineDiagnostic::UnsupportedChannel: return "unsupported-channel";
    case UiTimelineDiagnostic::TooManyKeys: return "too-many-keys";
    case UiTimelineDiagnostic::TooManyEvents: return "too-many-events";
    case UiTimelineDiagnostic::DuplicateEventId: return "duplicate-event-id";
    case UiTimelineDiagnostic::InvalidEvent: return "invalid-event";
    }
    return "unknown";
}

const char* UiTimelineStatusName(UiTimelineStatus value) noexcept {
    switch (value) {
    case UiTimelineStatus::Ok: return "ok";
    case UiTimelineStatus::NotConfigured: return "not-configured";
    case UiTimelineStatus::NotStarted: return "not-started";
    case UiTimelineStatus::NonFiniteTime: return "non-finite-time";
    case UiTimelineStatus::StaleGeneration: return "stale-generation";
    case UiTimelineStatus::GenerationZero: return "generation-zero";
    case UiTimelineStatus::GenerationNotNew: return "generation-not-new";
    case UiTimelineStatus::BackwardTime: return "backward-time";
    case UiTimelineStatus::NoSink: return "no-sink";
    }
    return "unknown";
}

void UiTimeline::ClearDelivery() noexcept {
    for (std::size_t index = 0; index < event_count_; ++index) {
        events_[index].delivered = false;
    }
    started_ = false;
    include_start_ = false;
}

void UiTimeline::Reset() noexcept {
    node_ = {};
    domain_ = UiTimeDomain::WallClock;
    keys_ = {};
    track_offset_ = {};
    track_count_ = {};
    events_ = {};
    event_count_ = 0;
    time_ms_ = 0.0;
    generation_ = 0;
    configured_ = false;
    ClearDelivery();
    // floor_generation_ and counters_ survive on purpose.
}

UiTimelineDiagnostic UiTimeline::Configure(
    UiNodeIdentity node, UiTimeDomain domain,
    std::span<const UiTimelineTrack> tracks,
    std::span<const UiTimelineEventSpec> events) noexcept {
    // Everything is validated into locals first: a rejection must leave the
    // previous configuration and playback state untouched.
    if (node.node == 0) return UiTimelineDiagnostic::InvalidKey;
    if (tracks.empty()) return UiTimelineDiagnostic::NoTracks;
    if (events.size() > kMaxEvents) return UiTimelineDiagnostic::TooManyEvents;
    if (domain != UiTimeDomain::WallClock && domain != UiTimeDomain::Song) {
        return UiTimelineDiagnostic::UnsupportedChannel;
    }

    std::array<UiKeyframe, kMaxKeys> keys{};
    std::array<std::uint32_t, kChannelCount> offsets{};
    std::array<std::uint32_t, kChannelCount> counts{};
    std::size_t total = 0;

    for (const auto& track : tracks) {
        const auto channel = static_cast<std::size_t>(track.channel);
        if (channel >= kChannelCount) return UiTimelineDiagnostic::UnsupportedChannel;
        if (counts[channel] != 0) return UiTimelineDiagnostic::UnsupportedChannel;
        if (track.keys.empty()) return UiTimelineDiagnostic::EmptyTrack;
        if (total + track.keys.size() > kMaxKeys) return UiTimelineDiagnostic::TooManyKeys;
        for (std::size_t index = 0; index < track.keys.size(); ++index) {
            const UiKeyframe& key = track.keys[index];
            if (!FiniteTime(key.time_ms) || !std::isfinite(key.value) ||
                std::abs(key.value) > kMaximumValue) {
                return UiTimelineDiagnostic::InvalidKey;
            }
            if (index != 0 && !(key.time_ms > track.keys[index - 1].time_ms)) {
                return UiTimelineDiagnostic::UnsortedKeys;
            }
            keys[total + index] = key;
        }
        offsets[channel] = static_cast<std::uint32_t>(total);
        counts[channel] = static_cast<std::uint32_t>(track.keys.size());
        total += track.keys.size();
    }

    std::array<Event, kMaxEvents> sorted{};
    std::size_t sorted_count = 0;
    for (const auto& spec : events) {
        if (!FiniteTime(spec.time_ms) || spec.id == 0) {
            return UiTimelineDiagnostic::InvalidEvent;
        }
        for (std::size_t index = 0; index < sorted_count; ++index) {
            if (sorted[index].id == spec.id) return UiTimelineDiagnostic::DuplicateEventId;
        }
        // Insertion sort: allocation free, and equal times keep source order.
        std::size_t position = sorted_count;
        while (position > 0 && sorted[position - 1].time_ms > spec.time_ms) {
            sorted[position] = sorted[position - 1];
            --position;
        }
        sorted[position] = Event{spec.time_ms, spec.id, false};
        ++sorted_count;
    }

    node_ = node;
    domain_ = domain;
    keys_ = keys;
    track_offset_ = offsets;
    track_count_ = counts;
    events_ = sorted;
    event_count_ = sorted_count;
    generation_ = 0;
    time_ms_ = 0.0;
    configured_ = true;
    ClearDelivery();
    return UiTimelineDiagnostic::None;
}

UiTimelineStatus UiTimeline::Start(std::uint64_t generation, double start_ms) noexcept {
    if (!configured_) return UiTimelineStatus::NotConfigured;
    if (!FiniteTime(start_ms)) return UiTimelineStatus::NonFiniteTime;
    if (generation == 0) return UiTimelineStatus::GenerationZero;
    if (generation <= floor_generation_) return UiTimelineStatus::GenerationNotNew;
    generation_ = generation;
    floor_generation_ = generation;
    time_ms_ = start_ms;
    started_ = true;
    include_start_ = true;
    for (auto& event : events_) event.delivered = false;
    ++counters_.starts;
    return UiTimelineStatus::Ok;
}

UiAdvanceOutcome UiTimeline::Advance(double now_ms, std::uint64_t generation,
                                     UiEventSink sink, void* context) noexcept {
    ++counters_.advances;
    if (!configured_) return {0, UiTimelineStatus::NotConfigured};
    if (!started_) return {0, UiTimelineStatus::NotStarted};
    if (!FiniteTime(now_ms)) {
        ++counters_.evaluate_failures;
        return {0, UiTimelineStatus::NonFiniteTime};
    }
    if (generation != generation_) {
        ++counters_.rejected_stale_generation;
        return {0, UiTimelineStatus::StaleGeneration};
    }
    if (!sink) {
        // Never consume an event without a consumer: time does not move, so the
        // crossings stay pending for the next call that supplies a sink.
        ++counters_.rejected_no_sink;
        return {0, UiTimelineStatus::NoSink};
    }
    if (now_ms < time_ms_) {
        ++counters_.rejected_backward;
        return {0, UiTimelineStatus::BackwardTime};
    }

    const double from = time_ms_;
    const bool include_start = include_start_;
    time_ms_ = now_ms;
    include_start_ = false;

    std::size_t delivered = 0;
    for (std::size_t index = 0; index < event_count_; ++index) {
        Event& event = events_[index];
        if (event.delivered) continue;
        const bool after_lower = include_start ? (event.time_ms >= from)
                                               : (event.time_ms > from);
        if (!after_lower || event.time_ms > now_ms) continue;
        event.delivered = true;
        sink(context, node_, event.id);
        ++delivered;
        ++counters_.events_delivered;
    }
    return {delivered, UiTimelineStatus::Ok};
}

bool UiTimeline::EvaluateTrack(const UiKeyframe* keys, std::size_t count,
                               double now_ms, float& value) noexcept {
    if (count == 0) return false;
    if (now_ms <= keys[0].time_ms) {
        value = keys[0].value;
        return true;
    }
    if (now_ms >= keys[count - 1].time_ms) {
        value = keys[count - 1].value;
        return true;
    }
    std::size_t index = 1;
    while (index < count && keys[index].time_ms <= now_ms) ++index;
    const UiKeyframe& previous = keys[index - 1];
    const UiKeyframe& next = keys[index];
    const double span = next.time_ms - previous.time_ms;
    if (!(span > 0.0)) {
        value = next.value;
        return true;
    }
    const double fraction = std::clamp((now_ms - previous.time_ms) / span, 0.0, 1.0);
    switch (previous.interpolation) {
    case UiInterpolation::Step:
        value = previous.value;
        break;
    case UiInterpolation::SmoothStep: {
        const float eased = SmoothStep(fraction);
        value = previous.value + (next.value - previous.value) * eased;
        break;
    }
    case UiInterpolation::Linear:
    default:
        value = previous.value +
            (next.value - previous.value) * static_cast<float>(fraction);
        break;
    }
    return std::isfinite(value);
}

bool UiTimeline::Evaluate(double now_ms, UiAnimationState& state) const noexcept {
    state = {};
    if (!configured_ || !FiniteTime(now_ms)) return false;
    bool any = false;
    for (std::size_t channel = 0; channel < kChannelCount; ++channel) {
        const std::uint32_t count = track_count_[channel];
        if (count == 0) continue;
        const UiKeyframe* keys = keys_.data() + track_offset_[channel];
        float value = 0.0f;
        if (!EvaluateTrack(keys, count, now_ms, value)) continue;
        if (ClampedChannel(static_cast<UiChannel>(channel))) {
            value = std::clamp(value, 0.0f, 1.0f);
        }
        state.values[channel] = value;
        state.valid[channel] = true;
        any = true;
    }
    return any;
}

// ---------------------------------------------------------------------------
// Bank

UiTimeline* UiTimelineBank::Acquire(UiNodeIdentity identity) noexcept {
    if (identity.node == 0) return nullptr;
    Entry* free_entry = nullptr;
    for (auto& entry : entries_) {
        if (entry.used && entry.timeline.Matches(identity)) return &entry.timeline;
        if (!entry.used && free_entry == nullptr) free_entry = &entry;
    }
    // Same address, different pool stamp: the node was recycled.
    for (auto& entry : entries_) {
        if (entry.used && entry.timeline.Node().node == identity.node) {
            entry.timeline.Reset();
            return &entry.timeline;
        }
    }
    if (free_entry == nullptr) return nullptr;
    free_entry->used = true;
    free_entry->timeline.Reset();
    return &free_entry->timeline;
}

UiTimeline* UiTimelineBank::Find(const UiNodeIdentity& identity) noexcept {
    for (auto& entry : entries_) {
        if (entry.used && entry.timeline.Matches(identity)) return &entry.timeline;
    }
    return nullptr;
}

void UiTimelineBank::Release(const UiNodeIdentity& identity) noexcept {
    for (auto& entry : entries_) {
        if (entry.used && entry.timeline.Node() == identity) {
            entry.timeline.Reset();
            entry.used = false;
        }
    }
}

void UiTimelineBank::ReleaseNode(std::uintptr_t node) noexcept {
    if (node == 0) return;
    for (auto& entry : entries_) {
        if (entry.used && entry.timeline.Node().node == node) {
            entry.timeline.Reset();
            entry.used = false;
        }
    }
}

void UiTimelineBank::Clear() noexcept {
    for (auto& entry : entries_) {
        entry.timeline.Reset();
        entry.used = false;
    }
    wall_ms_ = 0.0;
    song_ms_ = 0.0;
    song_generation_ = 0;
    wall_valid_ = false;
    song_valid_ = false;
}

void UiTimelineBank::SetWallClock(double ms) noexcept {
    if (!std::isfinite(ms) || ms < 0.0 || ms < wall_ms_) return;
    wall_ms_ = ms;
    wall_valid_ = true;
}

void UiTimelineBank::SetSongTime(double ms, std::uint64_t generation) noexcept {
    if (!std::isfinite(ms) || generation == 0) return;
    // The generation is a lifetime floor: an older producer must not rewind the
    // epoch even after the clock was cleared by a pause.
    if (generation < song_generation_) return;
    song_ms_ = ms;
    song_generation_ = generation;
    song_valid_ = true;
}

void UiTimelineBank::ClearSongTime() noexcept {
    song_valid_ = false;
}

std::size_t UiTimelineBank::AdvanceDomain(UiTimeDomain domain, UiEventSink sink,
                                          void* context) noexcept {
    const bool valid = domain == UiTimeDomain::WallClock ? wall_valid_ : song_valid_;
    if (!valid || !sink) return 0;
    const double now = domain == UiTimeDomain::WallClock ? wall_ms_ : song_ms_;
    const std::uint64_t generation =
        domain == UiTimeDomain::WallClock ? kWallGeneration : song_generation_;
    std::size_t delivered = 0;
    for (auto& entry : entries_) {
        if (!entry.used || entry.timeline.Domain() != domain) continue;
        if (entry.timeline.Generation() != generation) {
            // A new epoch (or a timeline acquired after the clock started):
            // prime it at the current time; an event exactly at `now` is
            // delivered by the inclusive start policy.
            if (entry.timeline.Start(generation, now) != UiTimelineStatus::Ok) {
                continue;
            }
        }
        delivered += entry.timeline.Advance(now, generation, sink, context).delivered;
    }
    return delivered;
}

std::size_t UiTimelineBank::Size() const noexcept {
    std::size_t count = 0;
    for (const auto& entry : entries_) {
        if (entry.used) ++count;
    }
    return count;
}

} // namespace nosfix
