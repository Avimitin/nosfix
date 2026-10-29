#pragma once

#include "nosfix/timing_events.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

// ===========================================================================
// Digital control normalization
// ===========================================================================
//
// This layer converts already-decoded physical control states into logical
// lane edges. It sits between the raw capture (which supplies QPC, sequence,
// ordinal and stream epoch) and any clock mapping or game judgment.
//
// It is deliberately NOT:
//   * HID report descriptor parsing. The decoder owns packet layout and
//     produces PhysicalControlState values with a stable device identity.
//   * Game judgment, scoring, key-sound dispatch or clock mapping. Events are
//     only stamped, filtered and aggregated; they are never interpreted.
//   * A polling loop. Reports are pushed in capture order; the consumer's
//     presentation cadence decides when Apply() is called, never what it
//     emits.
//
// Contract summary
// ----------------
// * Bindings are immutable while reports are being processed. Configure()
//   validates and installs a whole table or installs nothing; a rejected table
//   leaves the previous one active.
// * Every logical press and release is preserved exactly once. Duplicate make
//   reports (keyboard typematic / repeat), redundant held levels and velocity
//   changes while held produce no additional edge.
// * Lane state is the OR of all controls bound to that lane, so releasing one
//   of two held alternatives leaves the lane held.
// * Events are emitted in ascending lane order, each carrying the exact input
//   stamp and device identity of the control whose change triggered it.
// * Apply()/Reset() never allocate, lock, log, block or call any OS API. State
//   changes only on success; a rejected or capacity-short call leaves the
//   ordering cursor, the held state and the output span untouched so the caller
//   can retry the identical input.
//
// Finite bounds
// -------------
// kInputMaxBindings (256) caps the configured binding table. Each held entry
// corresponds to one distinct *bound* control, so the held-control table is
// bounded by the same number; no report can grow the tables.
//
// Threading
// ---------
// One owner calls Configure/Apply/Reset. Nothing here is atomic or locked; the
// integration must confine an instance to a single consumer thread (or
// serialize externally).

// Zero-based lane space used by the cabinet: 0..27.
inline constexpr std::uint8_t kInputLaneCount = 28;

// Maximum number of bindings in one configuration, and therefore the maximum
// number of distinct controls that can be held at once.
inline constexpr std::size_t kInputMaxBindings = 256;

// One control-to-lane association. The triple below is the exact mapping key:
// no wildcard, no default lane, no fallback. device_generation distinguishes a
// reconnected device from a previous one that reused the same source_id.
struct InputBinding final {
    std::uint64_t source_id = 0;
    std::uint64_t device_generation = 0;
    std::uint32_t control_id = 0;
    std::uint8_t lane = 0; // 0..kInputLaneCount-1
};

enum class InputConfigStatus : std::uint8_t {
    Accepted,
    RejectedTooManyBindings,   // bindings.size() > kInputMaxBindings
    RejectedInvalidBinding,    // zero source/generation/control, or lane >= 28
    RejectedDuplicateBinding,  // the same (source, generation, control, lane)
    RejectedHeldControlRebind, // would change the lanes of a currently held control
};

// Result of Apply(). See the per-value documentation for exact semantics.
enum class InputApplyStatus : std::uint8_t {
    // The report was valid, ordered and consumed. event_count lane edges were
    // written (possibly zero, e.g. for typematic repeats). The ordering cursor
    // advanced and control/lane state were updated.
    Applied,

    // The report was valid and ordered, but the control has no binding in the
    // current configuration. This is an explicit, expected outcome, not an
    // error: no lane is fabricated and no event is written. The report is still
    // consumed, so the ordering cursor advances. Callers that want to know
    // which controls are unmapped can count this status.
    IgnoredUnmappedControl,

    // Rejected: source_id/device_generation/control_id is zero, or velocity is
    // non-finite or outside [0, 1]. Nothing was consumed; the caller must not
    // treat the report as applied, and typically drops or repairs it.
    RejectedInvalidReport,

    // Rejected: stream_epoch is zero (unassigned capture stream), sequence is
    // zero, or qpc is negative. Nothing was consumed.
    RejectedInvalidStamp, // Also rejects changing QPC on an unclaimed reset identity.

    // Rejected: the report belongs to an older stream epoch than the adopted
    // one (a late report from an abandoned stream). Nothing was consumed.
    RejectedStaleEpoch,

    // Rejected: a newer epoch arrived without an explicit Reset(). Held state is
    // never carried across a loss boundary, so the caller must Reset() first.
    // Nothing was consumed.
    RejectedEpochChangeRequiresReset,

    // Rejected: within the current epoch, QPC decreased or (sequence, ordinal)
    // did not advance lexicographically. Equal QPC is valid; going backwards is
    // not. Nothing was consumed.
    RejectedBackwardStamp,

    // Duplicated delivery is rejected, never replayed. Identity is
    // (sequence, ordinal) within the epoch; a differing QPC on the same identity
    // is still a duplicate. Equality with the cursor is allowed only while the
    // cursor is a Reset() boundary marker that no report has claimed yet; once
    // claimed, the identity is spent permanently and cannot be reopened by any
    // later Reset().
    RejectedDuplicateStamp,

    // The report is valid and ordered but would emit required_event_count
    // edges, which does not fit in the supplied span. Nothing was consumed:
    // state, ordering cursor and the output span are untouched. Retry the
    // identical (state, stamp) with at least required_event_count slots.
    InsufficientOutputCapacity,
};

// Result of Reset(). Reset is the only way to cross a stream epoch.
enum class InputResetStatus : std::uint8_t {
    // Held lanes were released synthetically (possibly zero of them) and the
    // reset stamp was adopted as the new ordering cursor/epoch. A repeated
    // reset of a boundary marker that no report has claimed yet is also
    // Applied (with event_count 0): it is an idempotent retry with identical QPC.
    Applied,

    // Rejected: stream_epoch zero, sequence zero, or negative qpc. Nothing
    // changed, including held state.
    RejectedInvalidStamp, // Also rejects changing QPC on an unclaimed reset identity.

    // Rejected: the reset stamp names an epoch older than the adopted one.
    RejectedStaleEpoch,

    // Rejected: a same-epoch reset named an identity older than the cursor. A
    // genuine discontinuity must name a newer identity or a newer epoch.
    RejectedBackwardStamp,

    // Rejected: the same-epoch reset names the (sequence, ordinal) identity that
    // Apply() already consumed. A reset never reopens a consumed identity, so it
    // cannot arm a replay of that report. QPC is not part of identity: a
    // different QPC on the same identity is still rejected. Nothing changed,
    // including held state.
    RejectedDuplicateStamp,

    // The synthetic releases do not fit in the supplied span. Nothing changed:
    // lanes stay held and the epoch is not adopted. Retry with at least
    // required_event_count slots.
    InsufficientOutputCapacity,
};

struct InputApplyResult final {
    InputApplyStatus status = InputApplyStatus::RejectedInvalidReport;
    std::size_t event_count = 0;          // edges written (only for Applied)
    std::size_t required_event_count = 0; // see below

    // True when the report was accepted as part of the stream, whether or not
    // it produced edges. False for every rejection.
    [[nodiscard]] constexpr bool Consumed() const noexcept {
        return status == InputApplyStatus::Applied ||
               status == InputApplyStatus::IgnoredUnmappedControl;
    }
};

struct InputResetResult final {
    InputResetStatus status = InputResetStatus::RejectedInvalidStamp;
    std::size_t event_count = 0;          // synthetic releases written
    std::size_t required_event_count = 0; // see below

    [[nodiscard]] constexpr bool Applied() const noexcept {
        return status == InputResetStatus::Applied;
    }
};

// required_event_count semantics
// ------------------------------
// Apply(): set as soon as the report is known to be valid, ordered and mapped,
// i.e. for Applied and InsufficientOutputCapacity. It is the number of lane
// edges the report requires (equal to event_count when applied). It is zero for
// every rejection and for IgnoredUnmappedControl.
//
// Reset(): set as soon as the reset stamp is valid and ordered, i.e. for
// Applied and InsufficientOutputCapacity. It equals the number of currently
// held lanes, which is also the number of synthetic releases.
//
// The value is an upper bound a caller can pre-size with: it never exceeds
// kInputLaneCount.

class InputNormalizer final {
public:
    InputNormalizer() noexcept = default;

    // Installs a complete binding table. Transactional: the previous table
    // stays active unless this returns InputConfigStatus::Accepted.
    //
    // Validation order: size, per-binding fields, exact duplicates, then the
    // held-control rule. A control that is currently held may not be rebound:
    // its lane set in the new table must equal its lane set in the old one (and
    // it must still be present). This prevents a held control from silently
    // changing which lanes it holds, or from vanishing while it still holds a
    // lane. Reset() first; its synthetic releases clear held state and make the
    // rebind acceptable.
    //
    // Configuration is a startup/reconfiguration path and may copy the table; it
    // is also allocation-free here, so it may be called from any thread that
    // owns the instance.
    [[nodiscard]] InputConfigStatus Configure(std::span<const InputBinding> bindings) noexcept;

    // Consumes one decoded control report and writes its logical lane edges.
    //
    // Preconditions checked, in this order: report identity and velocity, stamp
    // fields, epoch/order against the consumed cursor, mapping, output capacity.
    // Nothing is mutated until every check has passed, and edges are written in
    // ascending lane order before the state commit.
    //
    // The very first valid report adopts its stream_epoch implicitly because no
    // held state exists yet (there is nothing to carry across a boundary). Every
    // later epoch change requires Reset().
    [[nodiscard]] InputApplyResult Apply(const PhysicalControlState& state,
                                         const InputStamp& stamp,
                                         std::span<LogicalInputEvent> out) noexcept;

    // Explicit stream boundary: loss/disconnect/retry, or the first report of a
    // new epoch. Writes one synthetic release per currently held lane, ordered
    // by lane, then clears all held state and adopts the reset stamp as the new
    // ordering cursor and epoch.
    //
    // The reset stamp is a boundary marker, not a consumed report: the first
    // Apply() after the reset may carry exactly the reset stamp (same sequence
    // and ordinal), which is the natural "reset at the first report of the new
    // stream, then process it" pattern. The marker is single-use. As soon as a
    // report claims that identity the identity is spent, and a later Reset()
    // naming it is rejected: a reset can never reopen a consumed identity and
    // therefore can never arm a replay of a physical press.
    //
    // Same-epoch resets must name a strictly newer identity than the cursor;
    // use a newer epoch for a genuine discontinuity. Repeating a reset while its
    // marker is still unclaimed is an idempotent no-op (Applied, no events).
    //
    // Synthetic releases:
    //   * stamp            = the reset stamp (exact, not synthesised time),
    //   * source_id/generation = the control that most recently contributed a
    //     press to that lane (provenance only; the stream epoch is new),
    //   * edge             = Release, velocity = 0, synthetic = true.
    // They exist so downstream state machines cannot be left holding a lane
    // across a gap. They are never physical judgments and must never score.
    [[nodiscard]] InputResetResult Reset(const InputStamp& stamp,
                                         std::span<LogicalInputEvent> out) noexcept;

    // ---- inspection (never mutates) ----

    [[nodiscard]] bool HasAdoptedEpoch() const noexcept { return epoch_ != 0; }
    [[nodiscard]] std::uint64_t Epoch() const noexcept { return epoch_; }

    // Last consumed (or adopted) report identity; all fields zero while no
    // epoch has been adopted.
    [[nodiscard]] const InputStamp& ConsumedStamp() const noexcept { return cursor_; }

    [[nodiscard]] std::size_t BindingCount() const noexcept { return binding_count_; }
    [[nodiscard]] std::size_t HeldControlCount() const noexcept { return held_count_; }

    // Bit i set => lane i is currently held by at least one bound control.
    [[nodiscard]] std::uint32_t HeldLaneMask() const noexcept { return held_lane_mask_; }

    [[nodiscard]] bool IsLaneHeld(std::uint8_t lane) const noexcept;
    [[nodiscard]] bool IsControlHeld(std::uint64_t source_id,
                                     std::uint64_t device_generation,
                                     std::uint32_t control_id) const noexcept;

private:
    // One entry per currently held control. Released controls are removed, so
    // the table never exceeds the number of distinct bound controls.
    struct HeldControl final {
        std::uint64_t source_id = 0;
        std::uint64_t device_generation = 0;
        std::uint32_t control_id = 0;
        float velocity = 0.0f; // last observed velocity; never used as an edge
    };

    // Provenance for synthetic releases only.
    struct LaneHolder final {
        std::uint64_t source_id = 0;
        std::uint64_t device_generation = 0;
    };

    [[nodiscard]] std::size_t FindHeld(std::uint64_t source_id,
                                       std::uint64_t device_generation,
                                       std::uint32_t control_id) const noexcept;
    void CommitCursor(const InputStamp& stamp) noexcept;

    std::array<InputBinding, kInputMaxBindings> bindings_{};
    std::size_t binding_count_ = 0;

    std::array<HeldControl, kInputMaxBindings> held_{};
    std::size_t held_count_ = 0;

    std::array<std::uint16_t, kInputLaneCount> lane_hold_count_{}; // held controls per lane
    std::array<LaneHolder, kInputLaneCount> lane_holder_{};
    std::uint32_t held_lane_mask_ = 0;

    InputStamp cursor_{}; // stream_epoch mirrors epoch_
    std::uint64_t epoch_ = 0; // 0 == no epoch adopted yet
    // True when cursor_ was established by a consumed report; false while it is
    // a Reset() boundary marker that no Apply() has claimed yet. The flag is
    // what keeps a reset from reopening an already-consumed identity.
    bool cursor_identity_consumed_ = false;
};

} // namespace nosfix
