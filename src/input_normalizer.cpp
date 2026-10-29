#include "nosfix/input_normalizer.hpp"

#include <array>
#include <cmath>

namespace nosfix {
namespace {

static_assert(kInputLaneCount > 0 && kInputLaneCount <= 32,
              "held_lane_mask_ is a 32-bit lane set");
static_assert(kInputMaxBindings > 0 && kInputMaxBindings <= 65535,
              "lane_hold_count_ is 16-bit");

[[nodiscard]] constexpr std::uint32_t LaneBit(std::uint8_t lane) noexcept {
    return std::uint32_t{1} << lane;
}

[[nodiscard]] constexpr bool IsValidLane(std::uint8_t lane) noexcept {
    return lane < kInputLaneCount;
}

// A binding key must be fully specified: there is no wildcard control, no
// default device generation and no fallback source.
[[nodiscard]] constexpr bool IsSpecified(std::uint64_t source_id,
                                         std::uint64_t device_generation,
                                         std::uint32_t control_id) noexcept {
    return source_id != 0 && device_generation != 0 && control_id != 0;
}

[[nodiscard]] bool VelocityValid(float velocity) noexcept {
    // Normalized magnitude. NaN/inf and out-of-range values indicate a broken
    // decoder and are rejected instead of being clamped silently.
    return std::isfinite(velocity) && velocity >= 0.0f && velocity <= 1.0f;
}

[[nodiscard]] constexpr bool StampFieldsValid(const InputStamp& stamp) noexcept {
    return stamp.stream_epoch != 0 && stamp.sequence != 0 && stamp.qpc >= 0;
}

// Within one epoch, reports are ordered lexicographically by (sequence,
// ordinal). Reports batched into one raw packet share sequence and QPC and are
// separated only by ordinal; distinct packets increase the sequence.
[[nodiscard]] constexpr bool StampKeyGreater(const InputStamp& later,
                                             const InputStamp& earlier) noexcept {
    if (later.sequence != earlier.sequence) return later.sequence > earlier.sequence;
    return later.ordinal > earlier.ordinal;
}

[[nodiscard]] constexpr bool SameReportIdentity(const InputStamp& a,
                                                const InputStamp& b) noexcept {
    return a.sequence == b.sequence && a.ordinal == b.ordinal;
}

[[nodiscard]] std::uint32_t LaneMaskForControl(std::span<const InputBinding> bindings,
                                               std::uint64_t source_id,
                                               std::uint64_t device_generation,
                                               std::uint32_t control_id) noexcept {
    std::uint32_t mask = 0;
    for (const InputBinding& binding : bindings) {
        if (binding.source_id == source_id &&
            binding.device_generation == device_generation &&
            binding.control_id == control_id) {
            mask |= LaneBit(binding.lane);
        }
    }
    return mask;
}

[[nodiscard]] std::size_t CountHeldLanes(const std::array<std::uint16_t, kInputLaneCount>& counts) noexcept {
    std::size_t held = 0;
    for (std::uint8_t lane = 0; lane < kInputLaneCount; ++lane) {
        if (counts[lane] != 0) ++held;
    }
    return held;
}

} // namespace

std::size_t InputNormalizer::FindHeld(std::uint64_t source_id,
                                      std::uint64_t device_generation,
                                      std::uint32_t control_id) const noexcept {
    for (std::size_t i = 0; i < held_count_; ++i) {
        const HeldControl& entry = held_[i];
        if (entry.source_id == source_id &&
            entry.device_generation == device_generation &&
            entry.control_id == control_id) {
            return i;
        }
    }
    return held_count_;
}

void InputNormalizer::CommitCursor(const InputStamp& stamp) noexcept {
    cursor_ = stamp;
    epoch_ = stamp.stream_epoch;
    // Apply() consumed this report; Reset() clears the flag again because its
    // own stamp is an unclaimed boundary marker.
    cursor_identity_consumed_ = true;
}

bool InputNormalizer::IsLaneHeld(std::uint8_t lane) const noexcept {
    return IsValidLane(lane) && (held_lane_mask_ & LaneBit(lane)) != 0;
}

bool InputNormalizer::IsControlHeld(std::uint64_t source_id,
                                    std::uint64_t device_generation,
                                    std::uint32_t control_id) const noexcept {
    return FindHeld(source_id, device_generation, control_id) < held_count_;
}

InputConfigStatus InputNormalizer::Configure(std::span<const InputBinding> bindings) noexcept {
    if (bindings.size() > kInputMaxBindings) {
        return InputConfigStatus::RejectedTooManyBindings;
    }

    // Build the candidate table in local storage first: no partial install is
    // possible, and the previous table is untouched on every rejection path.
    std::array<InputBinding, kInputMaxBindings> candidate{};
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        const InputBinding& binding = bindings[i];
        if (!IsSpecified(binding.source_id, binding.device_generation, binding.control_id) ||
            !IsValidLane(binding.lane)) {
            return InputConfigStatus::RejectedInvalidBinding;
        }
        for (std::size_t j = 0; j < i; ++j) {
            const InputBinding& previous = candidate[j];
            if (previous.source_id == binding.source_id &&
                previous.device_generation == binding.device_generation &&
                previous.control_id == binding.control_id &&
                previous.lane == binding.lane) {
                return InputConfigStatus::RejectedDuplicateBinding;
            }
        }
        candidate[i] = binding;
    }

    // A held control must keep exactly the lanes it currently holds. Changing
    // them would either orphan a lane (release never delivered) or invent a
    // press for a lane the user never touched. Reset() first: its synthetic
    // releases clear held state, after which any table is acceptable.
    const std::span<const InputBinding> previous_table{bindings_.data(), binding_count_};
    for (std::size_t i = 0; i < held_count_; ++i) {
        const HeldControl& entry = held_[i];
        const std::uint32_t before = LaneMaskForControl(previous_table, entry.source_id,
                                                        entry.device_generation,
                                                        entry.control_id);
        const std::uint32_t after = LaneMaskForControl(bindings, entry.source_id,
                                                       entry.device_generation,
                                                       entry.control_id);
        if (before != after) {
            return InputConfigStatus::RejectedHeldControlRebind;
        }
    }

    bindings_ = candidate;
    binding_count_ = bindings.size();
    return InputConfigStatus::Accepted;
}

InputApplyResult InputNormalizer::Apply(const PhysicalControlState& state,
                                        const InputStamp& stamp,
                                        std::span<LogicalInputEvent> out) noexcept {
    InputApplyResult result{};

    // 1. Report shape. A decoder must name its control exactly and supply a
    //    normalized velocity; a broken report is rejected, never guessed.
    if (!IsSpecified(state.source_id, state.device_generation, state.control_id) ||
        !VelocityValid(state.velocity)) {
        result.status = InputApplyStatus::RejectedInvalidReport;
        return result;
    }

    // 2. Stamp fields. Epoch 0 means "capture stream not established"; such a
    //    report cannot be ordered and is not silently adopted.
    if (!StampFieldsValid(stamp)) {
        result.status = InputApplyStatus::RejectedInvalidStamp;
        return result;
    }

    // 3. Epoch and in-epoch ordering against the consumed cursor.
    if (epoch_ != 0) {
        if (stamp.stream_epoch < epoch_) {
            result.status = InputApplyStatus::RejectedStaleEpoch;
            return result;
        }
        if (stamp.stream_epoch > epoch_) {
            // Never carry held state across a loss boundary.
            result.status = InputApplyStatus::RejectedEpochChangeRequiresReset;
            return result;
        }
        if (SameReportIdentity(stamp, cursor_)) {
            if (cursor_identity_consumed_) {
                // The report identity is unique and single-use within an epoch.
                // A repeat is a duplicated delivery, not new input, whatever its
                // payload or QPC says, and no later Reset can reopen it.
                result.status = InputApplyStatus::RejectedDuplicateStamp;
                return result;
            }
            if (stamp.qpc != cursor_.qpc) {
                result.status = InputApplyStatus::RejectedInvalidStamp;
                return result;
            }
            // The cursor is an unclaimed Reset() boundary marker: this report is
            // the first of the new stream and claims that identity exactly once.
        } else {
            if (stamp.qpc < cursor_.qpc) {
                result.status = InputApplyStatus::RejectedBackwardStamp;
                return result;
            }
            if (!StampKeyGreater(stamp, cursor_)) {
                result.status = InputApplyStatus::RejectedBackwardStamp;
                return result;
            }
        }
    }
    // No epoch adopted yet: this report establishes it. There is no held state
    // to carry across the boundary, so no Reset() is required for the first
    // report of the first stream.

    // 4. Mapping. The key is exact; an unbound control is an explicit ignored
    //    outcome with no fabricated lane.
    std::uint32_t lanes = 0;
    for (std::size_t i = 0; i < binding_count_; ++i) {
        const InputBinding& binding = bindings_[i];
        if (binding.source_id == state.source_id &&
            binding.device_generation == state.device_generation &&
            binding.control_id == state.control_id) {
            lanes |= LaneBit(binding.lane);
        }
    }
    if (lanes == 0) {
        CommitCursor(stamp); // consumed: the stream has advanced
        result.status = InputApplyStatus::IgnoredUnmappedControl;
        return result;
    }

    // 5. Edge detection. Duplicate make (typematic), redundant releases and
    //    velocity changes while held are all "no change".
    const std::size_t index = FindHeld(state.source_id, state.device_generation,
                                       state.control_id);
    const bool was_down = index < held_count_;
    if (was_down == state.down) {
        if (was_down) {
            held_[index].velocity = state.velocity; // tracked, but not an edge
        }
        CommitCursor(stamp);
        result.status = InputApplyStatus::Applied;
        return result;
    }

    // 6. Predicted lane transitions, computed before anything is mutated. A
    //    lane changes only when its held-control count crosses 0 or 1; that is
    //    what makes alternative bindings aggregate with OR semantics.
    std::array<std::uint8_t, kInputLaneCount> transition{}; // 0 none, 1 press, 2 release
    std::size_t required = 0;
    for (std::uint8_t lane = 0; lane < kInputLaneCount; ++lane) {
        if ((lanes & LaneBit(lane)) == 0) continue;
        const std::uint16_t count = lane_hold_count_[lane];
        if (state.down) {
            if (count == 0) {
                transition[lane] = 1;
                ++required;
            }
        } else if (count == 1) {
            transition[lane] = 2;
            ++required;
        }
    }
    result.required_event_count = required;
    if (out.size() < required) {
        // No hidden consumption: the caller retries the identical report.
        result.status = InputApplyStatus::InsufficientOutputCapacity;
        return result;
    }

    // 7. Emit in ascending lane order, carrying the exact stamp and identity of
    //    the control whose change triggered the edge.
    std::size_t written = 0;
    for (std::uint8_t lane = 0; lane < kInputLaneCount; ++lane) {
        const std::uint8_t edge = transition[lane];
        if (edge == 0) continue;
        LogicalInputEvent& event = out[written++];
        event.stamp = stamp;
        event.source_id = state.source_id;
        event.device_generation = state.device_generation;
        event.lane = lane;
        event.edge = edge == 1 ? InputEdge::Press : InputEdge::Release;
        event.velocity = edge == 1 ? state.velocity : 0.0f;
        event.synthetic = false;
    }

    // 8. Commit lane aggregation.
    for (std::uint8_t lane = 0; lane < kInputLaneCount; ++lane) {
        if ((lanes & LaneBit(lane)) == 0) continue;
        if (state.down) {
            ++lane_hold_count_[lane];
            lane_holder_[lane] = LaneHolder{state.source_id, state.device_generation};
            held_lane_mask_ |= LaneBit(lane);
        } else {
            --lane_hold_count_[lane];
            if (lane_hold_count_[lane] == 0) {
                lane_holder_[lane] = LaneHolder{};
                held_lane_mask_ &= ~LaneBit(lane);
            }
        }
    }

    // 9. Commit per-control state. The table only holds controls that are down,
    //    so it is bounded by the number of distinct bound controls, which is at
    //    most kInputMaxBindings (enforced by Configure).
    if (state.down) {
        held_[held_count_] = HeldControl{state.source_id, state.device_generation,
                                         state.control_id, state.velocity};
        ++held_count_;
    } else {
        held_[index] = held_[held_count_ - 1];
        --held_count_;
    }

    CommitCursor(stamp);
    result.status = InputApplyStatus::Applied;
    result.event_count = written;
    return result;
}

InputResetResult InputNormalizer::Reset(const InputStamp& stamp,
                                        std::span<LogicalInputEvent> out) noexcept {
    InputResetResult result{};

    if (!StampFieldsValid(stamp)) {
        result.status = InputResetStatus::RejectedInvalidStamp;
        return result;
    }

    if (epoch_ != 0) {
        if (stamp.stream_epoch < epoch_) {
            result.status = InputResetStatus::RejectedStaleEpoch;
            return result;
        }
        if (stamp.stream_epoch == epoch_) {
            if (SameReportIdentity(stamp, cursor_)) {
                if (cursor_identity_consumed_) {
                    // The identity was consumed by a report. Reopening it would
                    // let the caller re-Apply that report and emit a duplicate
                    // physical press, so the reset is refused outright.
                    result.status = InputResetStatus::RejectedDuplicateStamp;
                    return result;
                }
                if (stamp.qpc != cursor_.qpc) {
                    result.status = InputResetStatus::RejectedInvalidStamp;
                    return result;
                }
                // The exact unconsumed marker is idempotent. Its capture time
                // cannot be rewritten under the same identity.
                result.status = InputResetStatus::Applied;
                return result;
            } else {
                // A same-epoch reset is the retry/repair path, not a time
                // machine: it must name a strictly newer identity.
                if (stamp.qpc < cursor_.qpc) {
                    result.status = InputResetStatus::RejectedBackwardStamp;
                    return result;
                }
                if (!StampKeyGreater(stamp, cursor_)) {
                    result.status = InputResetStatus::RejectedBackwardStamp;
                    return result;
                }
            }
        }
        // A newer epoch restarts the ordering domain; its QPC only has to be
        // nonnegative (checked above).
    }

    const std::size_t required = CountHeldLanes(lane_hold_count_);
    result.required_event_count = required;
    if (out.size() < required) {
        // Held state survives so the caller can retry the identical reset.
        result.status = InputResetStatus::InsufficientOutputCapacity;
        return result;
    }

    // One synthetic release per held lane, ascending, using the reset stamp.
    std::size_t written = 0;
    for (std::uint8_t lane = 0; lane < kInputLaneCount; ++lane) {
        if (lane_hold_count_[lane] == 0) continue;
        LogicalInputEvent& event = out[written++];
        event.stamp = stamp;
        event.source_id = lane_holder_[lane].source_id;
        event.device_generation = lane_holder_[lane].device_generation;
        event.lane = lane;
        event.edge = InputEdge::Release;
        event.velocity = 0.0f;
        event.synthetic = true; // state recovery, never a physical judgment
    }

    held_count_ = 0;
    lane_hold_count_.fill(0);
    lane_holder_.fill(LaneHolder{});
    held_lane_mask_ = 0;
    CommitCursor(stamp);
    // The reset stamp marks the boundary; it is not a consumed report, so the
    // first Apply() after this may carry the same (sequence, ordinal), exactly
    // once.
    cursor_identity_consumed_ = false;

    result.status = InputResetStatus::Applied;
    result.event_count = written;
    return result;
}

} // namespace nosfix
