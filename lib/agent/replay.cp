// Origin: Pure-CP Stdlib (Deterministic Agent Offline Replay Cassette & Trajectory Diff Debugger) | Created: 2026-11-22
// Zero-heap-allocation deterministic offline replay engine and step-by-step trajectory diff
// comparator for millisecond-speed offline AI Agent regression debugging without live LLM calls.

const FNV1A_OFFSET_BASIS: u64 = 0xCBF29CE484222325;
const FNV1A_PRIME: u64 = 0x00000100000001B3;

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) return 0xFFFFFFFF;
    return a + b;
}

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Computes a deterministic 64-bit FNV-1a hash over serialized call arguments or prompt payload.
pub fn computeReplayArgsHash(args: string): u64 {
    let h: u64 = FNV1A_OFFSET_BASIS;
    let i: usize = 0;
    while (i < args.len) : (i += 1) {
        h = (h ^ (args[i] as u64)) *% FNV1A_PRIME;
    }
    return h;
}

/// Call category recorded in a replay cassette frame.
pub enum ReplayEntryKind {
    LlmCall,
    ToolCall,
}

/// Replay lookup matching policy.
pub enum ReplayMatchMode {
    /// Requires exact sequential order, kind, name, and args_hash match.
    StrictOrder,
    /// Matches by (kind, name, args_hash) anywhere in the cassette (starting from cursor).
    KeyedLoose,
}

/// Single recorded LLM or tool invocation frame in a `ReplayCassette`.
pub struct ReplayFrame {
    seq: u32 = 0;
    kind: ReplayEntryKind = ReplayEntryKind.LlmCall;
    name: string = "";
    args_hash: u64 = 0;
    response: string = "";
    latency_ms: u32 = 0;
    succeeded: bool = true;
}

/// Caller-backed zero-allocation recording buffer for offline agent replay cassettes.
pub struct ReplayCassette {
    buf: []ReplayFrame;
    len: usize = 0;

    /// Initializes a cassette over caller-owned frame storage `buf`.
    static fn init(buf: []ReplayFrame): ReplayCassette {
        return ReplayCassette{
            buf: buf,
            len: 0,
        };
    }

    /// Appends a frame with a precomputed `args_hash`. Returns `false` if the cassette is full.
    fn record(
        *_,
        kind: ReplayEntryKind,
        name: string,
        args_hash: u64,
        response: string,
        latency_ms: u32,
        succeeded: bool,
    ): bool {
        if (_.len >= _.buf.len) {
            return false;
        }
        _.buf[_.len] = ReplayFrame{
            seq: _.len as u32,
            kind: kind,
            name: name,
            args_hash: args_hash,
            response: response,
            latency_ms: latency_ms,
            succeeded: succeeded,
        };
        _.len += 1;
        return true;
    }

    /// Appends a frame by hashing raw `args` with `computeReplayArgsHash`.
    /// Returns `false` if the cassette is full.
    fn recordRawArgs(
        *_,
        kind: ReplayEntryKind,
        name: string,
        args: string,
        response: string,
        latency_ms: u32,
        succeeded: bool,
    ): bool {
        return _.record(kind, name, computeReplayArgsHash(args), response, latency_ms, succeeded);
    }

    /// Returns the number of recorded frames.
    fn count(): usize {
        return _.len;
    }

    /// Returns the maximum frame capacity of the backing buffer.
    fn capacity(): usize {
        return _.buf.len;
    }

    /// Returns `true` when no further frames can be recorded.
    fn isFull(): bool {
        return _.len >= _.buf.len;
    }

    /// Returns the frame at `idx`, or `null` if out of bounds.
    fn frameAt(idx: usize): ReplayFrame? {
        if (idx >= _.len) return null;
        return _.buf[idx];
    }

    /// Returns a read-only slice of all recorded frames in chronological order.
    fn frames(): []const ReplayFrame {
        return _.buf[0.._.len];
    }

    /// Resets the recorded frame count to zero without reallocating.
    fn clear(*_) {
        _.len = 0;
    }
}

/// Replay execution hit/miss telemetry snapshot.
pub struct ReplayStats {
    hits: u32 = 0;
    misses: u32 = 0;
    arg_mismatches: u32 = 0;
    cursor: usize = 0;
    total_frames: usize = 0;
}

/// Deterministic offline replay player supporting strict sequential and loose keyed playback.
pub struct ReplayPlayer {
    frames: []const ReplayFrame;
    mode: ReplayMatchMode = ReplayMatchMode.StrictOrder;
    cursor: usize = 0;
    hits: u32 = 0;
    misses: u32 = 0;
    arg_mismatches: u32 = 0;

    /// Creates a new `ReplayPlayer` over `frames` using `mode`.
    static fn init(frames: []const ReplayFrame, mode: ReplayMatchMode): ReplayPlayer {
        return ReplayPlayer{
            frames: frames,
            mode: mode,
            cursor: 0,
            hits: 0,
            misses: 0,
            arg_mismatches: 0,
        };
    }

    /// Resets playback cursor and counters to the beginning of the cassette.
    fn reset(*_) {
        _.cursor = 0;
        _.hits = 0;
        _.misses = 0;
        _.arg_mismatches = 0;
    }

    /// Returns a snapshot of playback statistics.
    fn stats(): ReplayStats {
        return ReplayStats{
            hits: _.hits,
            misses: _.misses,
            arg_mismatches: _.arg_mismatches,
            cursor: _.cursor,
            total_frames: _.frames.len,
        };
    }

    /// Returns how many frames remain from `cursor` to the end of the cassette.
    fn remaining(): usize {
        if (_.cursor >= _.frames.len) return 0;
        return _.frames.len - _.cursor;
    }

    /// Returns `true` when `cursor` has reached the end of the cassette.
    fn isExhausted(): bool {
        return _.cursor >= _.frames.len;
    }

    /// Replays the matching frame for `(kind, name, args_hash)` according to `mode`.
    /// Returns `null` on miss or argument hash drift, updating `misses` or `arg_mismatches`.
    fn replay(*_, kind: ReplayEntryKind, name: string, args_hash: u64): ReplayFrame? {
        if (_.mode == ReplayMatchMode.StrictOrder) {
            if (_.cursor >= _.frames.len) {
                _.misses = satAddU32(_.misses, 1);
                return null;
            }
            const f = _.frames[_.cursor];
            if (f.kind != kind || !strEq(f.name, name)) {
                _.misses = satAddU32(_.misses, 1);
                return null;
            }
            if (f.args_hash != args_hash) {
                _.arg_mismatches = satAddU32(_.arg_mismatches, 1);
                return null;
            }
            _.hits = satAddU32(_.hits, 1);
            _.cursor += 1;
            return f;
        }

        // KeyedLoose mode: scan from cursor..len then wrap 0..cursor for exact (kind, name, args_hash)
        const n = _.frames.len;
        if (n == 0) {
            _.misses = satAddU32(_.misses, 1);
            return null;
        }
        const start = if (_.cursor < n) _.cursor else 0;
        let saw_name_match = false;
        let offset: usize = 0;
        while (offset < n) : (offset += 1) {
            const idx = (start + offset) % n;
            const f = _.frames[idx];
            if (f.kind == kind && strEq(f.name, name)) {
                if (f.args_hash == args_hash) {
                    _.hits = satAddU32(_.hits, 1);
                    _.cursor = idx + 1;
                    return f;
                }
                saw_name_match = true;
            }
        }

        if (saw_name_match) {
            _.arg_mismatches = satAddU32(_.arg_mismatches, 1);
        } else {
            _.misses = satAddU32(_.misses, 1);
        }
        return null;
    }

    /// Convenience helper that hashes raw `args` with `computeReplayArgsHash` before replay lookup.
    fn replayRawArgs(*_, kind: ReplayEntryKind, name: string, args: string): ReplayFrame? {
        return _.replay(kind, name, computeReplayArgsHash(args));
    }
}

// ============================================================================
// Step-by-Step Trajectory Diff Comparator
// ============================================================================

/// Classification of a single step comparison between baseline and candidate trajectories.
pub enum ReplayDiffKind {
    Matched,
    KindMismatch,
    NameMismatch,
    ArgsMismatch,
    MissingStep,
    ExtraStep,
}

/// Detailed divergence record for a single trajectory step.
pub struct ReplayStepDiff {
    step_index: usize = 0;
    kind: ReplayDiffKind = ReplayDiffKind.Matched;
    baseline_kind: ReplayEntryKind = ReplayEntryKind.LlmCall;
    candidate_kind: ReplayEntryKind = ReplayEntryKind.LlmCall;
    baseline_name: string = "";
    candidate_name: string = "";
    baseline_args_hash: u64 = 0;
    candidate_args_hash: u64 = 0;
}

/// Aggregate trajectory diff summary including first divergence index and fidelity score `[0.0, 1.0]`.
pub struct ReplayDiffSummary {
    baseline_len: usize = 0;
    candidate_len: usize = 0;
    compared_steps: usize = 0;
    matched_steps: usize = 0;
    mismatch_steps: usize = 0;
    missing_steps: usize = 0;
    extra_steps: usize = 0;
    has_divergence: bool = false;
    first_divergence_step: usize = 0;
    diffs_written: usize = 0;
    diffs_truncated: bool = false;
    fidelity_score: f64 = 1.0;

    /// Returns `true` when baseline and candidate trajectories match at every step.
    fn isExactMatch(): bool {
        return !_.has_divergence;
    }
}

fn pushStepDiff(out_diffs: []ReplayStepDiff, summary: *ReplayDiffSummary, rec: ReplayStepDiff) {
    if (!summary.*.has_divergence) {
        summary.*.has_divergence = true;
        summary.*.first_divergence_step = rec.step_index;
    }
    if (summary.*.diffs_written < out_diffs.len) {
        out_diffs[summary.*.diffs_written] = rec;
        summary.*.diffs_written += 1;
    } else {
        summary.*.diffs_truncated = true;
    }
}

/// Compares `baseline` and `candidate` trajectories frame by frame without heap allocation.
/// Writes non-matching step records into `out_diffs` and returns a complete `ReplayDiffSummary`
/// with `first_divergence_step` and `fidelity_score = matched_steps / max(baseline.len, candidate.len)`.
pub fn diffReplayTrajectories(
    baseline: []const ReplayFrame,
    candidate: []const ReplayFrame,
    out_diffs: []ReplayStepDiff,
): ReplayDiffSummary {
    const total = if (baseline.len >= candidate.len) baseline.len else candidate.len;
    let summary = ReplayDiffSummary{
        baseline_len: baseline.len,
        candidate_len: candidate.len,
        compared_steps: total,
        matched_steps: 0,
        mismatch_steps: 0,
        missing_steps: 0,
        extra_steps: 0,
        has_divergence: false,
        first_divergence_step: 0,
        diffs_written: 0,
        diffs_truncated: false,
        fidelity_score: 1.0,
    };

    if (total == 0) {
        return summary;
    }

    let i: usize = 0;
    while (i < total) : (i += 1) {
        if (i < baseline.len && i < candidate.len) {
            const b = baseline[i];
            const c = candidate[i];
            if (b.kind != c.kind) {
                summary.mismatch_steps += 1;
                pushStepDiff(out_diffs, *summary, ReplayStepDiff{
                    step_index: i,
                    kind: ReplayDiffKind.KindMismatch,
                    baseline_kind: b.kind,
                    candidate_kind: c.kind,
                    baseline_name: b.name,
                    candidate_name: c.name,
                    baseline_args_hash: b.args_hash,
                    candidate_args_hash: c.args_hash,
                });
            } else if (!strEq(b.name, c.name)) {
                summary.mismatch_steps += 1;
                pushStepDiff(out_diffs, *summary, ReplayStepDiff{
                    step_index: i,
                    kind: ReplayDiffKind.NameMismatch,
                    baseline_kind: b.kind,
                    candidate_kind: c.kind,
                    baseline_name: b.name,
                    candidate_name: c.name,
                    baseline_args_hash: b.args_hash,
                    candidate_args_hash: c.args_hash,
                });
            } else if (b.args_hash != c.args_hash) {
                summary.mismatch_steps += 1;
                pushStepDiff(out_diffs, *summary, ReplayStepDiff{
                    step_index: i,
                    kind: ReplayDiffKind.ArgsMismatch,
                    baseline_kind: b.kind,
                    candidate_kind: c.kind,
                    baseline_name: b.name,
                    candidate_name: c.name,
                    baseline_args_hash: b.args_hash,
                    candidate_args_hash: c.args_hash,
                });
            } else {
                summary.matched_steps += 1;
            }
        } else if (i < baseline.len) {
            const b = baseline[i];
            summary.missing_steps += 1;
            pushStepDiff(out_diffs, *summary, ReplayStepDiff{
                step_index: i,
                kind: ReplayDiffKind.MissingStep,
                baseline_kind: b.kind,
                candidate_kind: b.kind,
                baseline_name: b.name,
                candidate_name: "",
                baseline_args_hash: b.args_hash,
                candidate_args_hash: 0,
            });
        } else {
            const c = candidate[i];
            summary.extra_steps += 1;
            pushStepDiff(out_diffs, *summary, ReplayStepDiff{
                step_index: i,
                kind: ReplayDiffKind.ExtraStep,
                baseline_kind: c.kind,
                candidate_kind: c.kind,
                baseline_name: "",
                candidate_name: c.name,
                baseline_args_hash: 0,
                candidate_args_hash: c.args_hash,
            });
        }
    }

    summary.fidelity_score = (summary.matched_steps as f64) / (total as f64);
    return summary;
}
