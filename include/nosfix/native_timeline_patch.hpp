#pragma once

// Guarded native timeline conversion patches.
//
// The patch set changes only instruction operands and reset/window constants:
//   * 43 float rate*rate sites load 60,000,000.0 us/minute and NOP the square;
//   * 16 double-rate reads in the chart loader/main loop/cue helper load
//     1,000,000.0;
//   * 6 counter resets write the microsecond preroll;
//   * 8 judgment windows are rewritten to microsecond boundaries.
//
// Every site is validated against the analyzed image before any byte is
// written. Prepare allocates the constants page and builds the full change set;
// Commit applies it as one transaction with rollback on failure.

#include <Windows.h>

#include <string>

namespace nosfix {

[[nodiscard]] bool PrepareTimelineConversionPatches(HMODULE module,
                                                     std::string& reason);
[[nodiscard]] bool CommitTimelineConversionPatches(std::string& reason);
void RollbackTimelineConversionPatches() noexcept;

} // namespace nosfix
