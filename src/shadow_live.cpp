#include "nosfix/shadow_live.hpp"

#include "nosfix/log.hpp"

#include <Windows.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace nosfix {
namespace {

constexpr std::size_t kMaxShadowNotes = 512;
constexpr std::uint64_t kNoteIdleTicks = 240;
constexpr std::size_t kOutcomeLogLimit = 2000;

bool Readable(const void* address, std::size_t bytes) noexcept {
    if (!address || bytes == 0) return false;
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(address, &info, sizeof(info)) == 0) return false;
    if (info.State != MEM_COMMIT) return false;
    if ((info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) return false;
    const auto start = reinterpret_cast<std::uintptr_t>(address);
    const auto end = start + bytes;
    const auto region_end = reinterpret_cast<std::uintptr_t>(info.BaseAddress) +
                            info.RegionSize;
    return end >= start && end <= region_end;
}

template <class T>
T Read(const void* base, std::size_t offset) noexcept {
    T value{};
    if (!base) return value;
    std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(T));
    return value;
}

struct NoteEntry final {
    const void* note = nullptr;
    const void* chart = nullptr;
    std::uint32_t id = 0;
    std::uint64_t last_tick = 0;
    MatchCandidate candidate{};
};

bool g_enabled = false;
bool g_pending = false;
const std::byte* g_image = nullptr;
std::size_t g_image_size = 0;
const void* g_module = nullptr;
NativeShadowFeed g_feed;
ShadowFeedConfig g_config{};
std::array<NoteEntry, kMaxShadowNotes> g_notes{};
std::uint64_t g_tick = 0;
std::uint32_t g_next_id = 1;
std::uint64_t g_outcome_logs = 0;
ShadowFeedStatus g_last_error = ShadowFeedStatus::NotConfigured;
ShadowLiveMetrics g_metrics{};

NativeMatchEvidence ShadowMatchEvidence() noexcept {
    // This is a comparison source, not the judge. The R2 model has decoded the
    // listed facts; the two live-only rules remain feature-gated in
    // NativeShadowEvidence below.
    NativeMatchEvidence evidence;
    evidence.horizon_writers_known = true;
    evidence.pair_record_layout_known = true;
    evidence.bucket_table_known = true;
    evidence.linked_object_known = true;
    evidence.linked_pairs_never_judged = true;
    evidence.slot_priority_known = true;
    evidence.per_group_masks_known = true;
    evidence.horizon_push_sites_known = true;
    evidence.auto_slot_rule_known = true;
    evidence.over_filter_rule_known = true;
    return evidence;
}

NativeShadowEvidence ShadowEvidence() noexcept {
    NativeShadowEvidence evidence;
    evidence.bucket_layout_known = true;
    evidence.bucket_rotation_known = true;
    evidence.bucket_compaction_known = true;
    evidence.candidate_ordering_known = true;
    evidence.note_field_layout_known = true;
    evidence.song_unit_rate_known = true;
    // note+84 > 1 and the phase-2 group window remain live-only.
    evidence.slot_override_known = false;
    evidence.group_phase_known = false;
    return evidence;
}

bool BuildConfig() noexcept {
    if (!g_image || g_image_size < 0x1366908) return false;
    const auto sample = SampleJudgmentWindows(
        std::span<const std::byte>(g_image, g_image_size),
        kProvenSongUnitsPerSecond, true);
    if (!sample.ok()) return false;

    ShadowFeedConfig config;
    config.match.windows = sample.windows;
    config.match.units_per_second = kProvenSongUnitsPerSecond;
    config.match.layout_sampled = true;
    config.match.horizon.sampled = true;
    config.match.lane_over_filter = Read<std::int32_t>(g_image, 0x13E4A84);
    if (config.match.lane_over_filter < 0 || config.match.lane_over_filter > 64)
        config.match.lane_over_filter = 3;
    const float spread = Read<float>(g_image, 0x13E5724);
    config.match.key_spread =
        std::isfinite(spread) && spread > 0.0f && spread <= 1024.0f ? spread : 0.4f;
    config.match.effect_early_units =
        Read<std::int32_t>(g_image, 0x13668E0);
    config.match.judgment_offset_us = SongUnitsToUs(
        static_cast<double>(Read<std::int32_t>(g_image, 0x136690C)),
        kProvenSongUnitsPerSecond);
    config.match.visual_offset_us = SongUnitsToUs(
        static_cast<double>(Read<std::int32_t>(g_image, 0x1366918)),
        kProvenSongUnitsPerSecond);
    config.match_evidence = ShadowMatchEvidence();
    config.evidence = ShadowEvidence();
    config.scoring = NativeScoringConfig{};
    config.compact = NativeBucketCompactConfig{};
    config.auto_mode = false;
    config.auto_play = false;
    config.effects_muted = false;
    g_config = config;
    return true;
}

NoteEntry* FindEntry(const void* note) noexcept {
    for (auto& entry : g_notes) {
        if (entry.note == note) return &entry;
    }
    return nullptr;
}

NoteEntry* AllocateEntry() noexcept {
    for (auto& entry : g_notes) {
        if (!entry.note) return &entry;
    }
    // Evict the least recently observed note.
    NoteEntry* oldest = &g_notes[0];
    for (auto& entry : g_notes) {
        if (entry.last_tick < oldest->last_tick) oldest = &entry;
    }
    oldest->note = nullptr;
    return oldest;
}

void LogFeedError(ShadowFeedStatus status) noexcept {
    if (status == g_last_error) return;
    g_last_error = status;
    char line[192]{};
    std::snprintf(line, sizeof(line), "SHADOW feed status=%s",
                  ShadowFeedStatusName(status));
    Log(LogLevel::Warning, line);
}

} // namespace

bool TryConfigureShadowLive() noexcept {
    if (!BuildConfig()) return false;
    const ShadowFeedStatus status = g_feed.Configure(g_config);
    g_metrics.last_status = status;
    if (status != ShadowFeedStatus::Ok) {
        LogFeedError(status);
        return false;
    }
    g_enabled = true;
    g_pending = false;
    g_last_error = ShadowFeedStatus::Ok;
    Log(LogLevel::Info, "SHADOW live feed configured (normal notes, diagnostic only)");
    return true;
}

void ConfigureShadowLive(const std::byte* image, std::size_t image_size,
                         const void* module_base) noexcept {
    g_enabled = false;
    g_pending = false;
    g_image = image;
    g_image_size = image_size;
    g_module = module_base;
    g_tick = 0;
    g_next_id = 1;
    g_outcome_logs = 0;
    g_notes.fill(NoteEntry{});
    g_metrics = ShadowLiveMetrics{};
    if (!TryConfigureShadowLive()) {
        // The SDK entry point runs before the game's judgment-window
        // initializer. Retry once per tick until the block is non-zero.
        g_pending = true;
        Log(LogLevel::Info, "SHADOW live feed waiting for the judgment-window sample");
    }
}

void DisableShadowLive() noexcept {
    g_enabled = false;
    g_pending = false;
}

bool ShadowLiveEnabled() noexcept { return g_enabled; }
bool ShadowLivePending() noexcept { return g_pending; }

void ShadowLiveObserveNormalNote(void* note) noexcept {
    if (!g_enabled || !Readable(note, 0xE0)) return;
    const auto* chart = Read<const void*>(note, 8);
    if (!chart || !Readable(chart, 0x68)) return;
    if (Read<std::int32_t>(note, 68) != 0) {
        // Keep the candidate for the final tick; done is reflected below.
    }

    NoteEntry* entry = FindEntry(note);
    if (entry && entry->chart != chart) entry = nullptr; // pointer reused
    if (entry && g_tick - entry->last_tick > kNoteIdleTicks) entry = nullptr;
    if (!entry) {
        entry = AllocateEntry();
        entry->note = note;
        entry->chart = chart;
        entry->id = g_next_id++;
        if (g_next_id == 0) g_next_id = 1;
        ++g_metrics.observed_notes;
    }
    entry->last_tick = g_tick;

    NativeChartFields chart_fields;
    chart_fields.group = Read<std::int32_t>(chart, 0x08);
    chart_fields.lane = Read<std::int32_t>(chart, 0x10);
    chart_fields.frame = Read<std::int32_t>(chart, 0x14);
    chart_fields.keysound_id = Read<std::int32_t>(chart, 0x30);
    chart_fields.chord_key = Read<std::int32_t>(chart, 0x38);
    chart_fields.left_key = Read<std::int32_t>(chart, 0x58);
    chart_fields.right_key = Read<std::int32_t>(chart, 0x5C);
    chart_fields.auto_play = Read<std::uint8_t>(chart, 0x61) != 0;
    chart_fields.sprite_select = Read<std::int32_t>(chart, 0x64);

    std::uint32_t slot = static_cast<std::uint32_t>(Read<std::int32_t>(note, 84));
    if (slot < 1 || slot > 3) slot = 1;
    NativeNoteFields note_fields;
    if (DecodeNormalNote(chart_fields, slot, 0, false, false, note_fields) !=
        NoteDecodeStatus::Ok) {
        entry->note = nullptr;
        return;
    }
    MatchCandidate candidate;
    if (MakeMatchCandidate(chart_fields, note_fields, entry->id,
                           g_config.match.units_per_second, candidate) !=
        NoteDecodeStatus::Ok) {
        entry->note = nullptr;
        return;
    }
    candidate.done = Read<std::int32_t>(note, 68) != 0;
    candidate.pending_early = Read<std::int32_t>(note, 76) != 0;
    candidate.consumed = Read<std::int32_t>(chart, 84) > 0;
    candidate.auto_slot_empty = Read<const void*>(note, 96) == nullptr;
    entry->candidate = candidate;
}

void ShadowLiveTick(double now_us, const void* module_base) noexcept {
    if (!std::isfinite(now_us)) return;
    if (!g_enabled) {
        if (!g_pending || !TryConfigureShadowLive()) return;
    }
    const void* module = g_module ? g_module : module_base;
    const auto* staging = Read<const void*>(module, 0x13DEA10);
    if (!staging || !Readable(staging, 784)) {
        ++g_metrics.feed_errors;
        return;
    }

    ++g_tick;
    for (auto& entry : g_notes) {
        if (entry.note && g_tick - entry.last_tick > kNoteIdleTicks)
            entry.note = nullptr;
    }

    std::array<MatchCandidate, kMaxShadowNotes> candidates{};
    std::size_t candidate_count = 0;
    for (const auto& entry : g_notes) {
        if (entry.note) candidates[candidate_count++] = entry.candidate;
    }

    std::array<NativeBucketSlot, kNativeBucketSlotCount> slots{};
    for (std::size_t i = 0; i < slots.size(); ++i) {
        const std::size_t base = i * kNativeBucketSlotBytes;
        slots[i].key = Read<std::int32_t>(staging, base + kNativeSlotKeyOffset);
        slots[i].state = Read<std::int32_t>(staging, base + kNativeSlotStateOffset);
        slots[i].value = Read<std::uint8_t>(staging, base + kNativeSlotValueOffset);
        slots[i].group = Read<std::int32_t>(staging, base + kNativeSlotGroupOffset);
        slots[i].repeat = Read<std::int32_t>(staging, base + kNativeSlotRepeatOffset);
    }

    ShadowFeedStatus status = g_feed.BeginFrame(now_us);
    if (status == ShadowFeedStatus::TimeBackwards) {
        // A seek/retry reopens the shadow timeline; the original judge is
        // untouched. Keep the note identities but rebuild the feed state.
        const ShadowFeedStatus reconfigured = g_feed.Configure(g_config);
        if (reconfigured != ShadowFeedStatus::Ok) {
            ++g_metrics.feed_errors;
            g_metrics.last_status = reconfigured;
            LogFeedError(reconfigured);
            return;
        }
        status = g_feed.BeginFrame(now_us);
    }
    if (status != ShadowFeedStatus::Ok) {
        ++g_metrics.feed_errors;
        g_metrics.last_status = status;
        LogFeedError(status);
        return;
    }

    std::array<NativeNoteOutcome, kNativeBucketSlotCount> outcomes{};
    std::array<ScoreEffect, kNativeBucketSlotCount> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    status = g_feed.FeedFrame(
        std::span<const NativeBucketSlot>(slots),
        std::span<const MatchCandidate>(candidates.data(), candidate_count),
        outcomes, outcome_count, effects, effect_count);
    g_metrics.last_status = status;
    if (status != ShadowFeedStatus::Ok) {
        ++g_metrics.feed_errors;
        LogFeedError(status);
        return;
    }
    g_last_error = ShadowFeedStatus::Ok;
    ++g_metrics.frames;
    g_metrics.outcomes += outcome_count;
    g_metrics.effects += effect_count;

    if (g_outcome_logs < kOutcomeLogLimit) {
        for (std::size_t i = 0; i < outcome_count; ++i) {
            const auto& outcome = outcomes[i];
            char line[256]{};
            std::snprintf(line, sizeof(line),
                "SHADOW outcome note=%u lane=%u kind=%u rank=%u grade=%u "
                "at=%.3fms input=%.3fms",
                outcome.note_index, outcome.lane,
                static_cast<unsigned>(outcome.kind),
                static_cast<unsigned>(outcome.rank), outcome.effect_grade,
                outcome.at_us / 1000.0, outcome.input_us / 1000.0);
            Log(LogLevel::Info, line);
            ++g_outcome_logs;
        }
        for (std::size_t i = 0; i < effect_count; ++i) {
            const auto& effect = effects[i];
            char line[256]{};
            std::snprintf(line, sizeof(line),
                "SHADOW effect note=%u grade=%u score=%d combo=%d->%d "
                "overridden=%u",
                effect.note_index, effect.grade, effect.score,
                effect.combo_before, effect.combo_after,
                effect.grade_overridden ? 1u : 0u);
            Log(LogLevel::Info, line);
            ++g_outcome_logs;
        }
    }
}

ShadowLiveMetrics TakeShadowLiveMetrics() noexcept {
    const ShadowLiveMetrics result = g_metrics;
    g_metrics.frames = 0;
    g_metrics.observed_notes = 0;
    g_metrics.outcomes = 0;
    g_metrics.effects = 0;
    g_metrics.feed_errors = 0;
    return result;
}

} // namespace nosfix
