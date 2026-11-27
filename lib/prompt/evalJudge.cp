// Origin: Pure-CP Stdlib (Zero-Alloc LLM-as-a-Judge Rubric Evaluator & Anti-Position-Bias Pairwise Arena) | Created: 2026-11-23
// Pure CP zero-heap-allocation LLM-as-a-Judge evaluation & release-gating engine:
//   - Multi-dimensional weighted Rubric (up to 16 dimensions, normalized 0..10000 bps score,
//     per-dimension min_score & hard_gate veto, strict bounds validation)
//   - Position-bias-resistant pairwise A/B verdict resolver (`resolvePairwiseBias`: merges
//     forward (A,B) and swapped (B,A) verdicts into win_a | win_b | tie | inconsistent_bias)
//   - Zero-heap multi-model Elo Arena (`JudgeArena`: up to 16 models, standard Elo updates,
//     automatic rejection of inconsistent_bias dirty samples, sorted leaderboard & telemetry)
//   - Zero-alloc JSON release-gate summary formatter

const std = @import("std");

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of evaluation rubric dimensions stored inline.
pub const JUDGE_MAX_DIMENSIONS: usize = 16;

/// Maximum number of candidate models tracked in `JudgeArena`.
pub const JUDGE_MAX_MODELS: usize = 16;

/// Maximum byte length of a dimension or model display name.
pub const JUDGE_NAME_MAX_LEN: usize = 48;

/// Basis-points scale (10000 bps == 100.00%).
pub const JUDGE_BPS_SCALE: u32 = 10000;

/// Default initial Elo rating for newly registered models.
pub const JUDGE_DEFAULT_ELO: f64 = 1500.0;

/// Default Elo K-factor.
pub const JUDGE_DEFAULT_K_FACTOR: f64 = 32.0;

/// Single evaluation dimension within a `JudgeRubric`.
pub struct JudgeDimensionSpec {
    active: bool = false;
    name_buf: [JUDGE_NAME_MAX_LEN]u8 = [0; JUDGE_NAME_MAX_LEN];
    name_len: usize = 0;
    weight: u32 = 1;
    max_score: u32 = 5;
    min_score: u32 = 0;
    hard_gate: bool = false;

    pub fn nameSlice(): []const u8 {
        return name_buf[0..name_len];
    }
}

/// Evaluation outcome produced by `JudgeRubric.evaluate`.
pub struct RubricEvalReport {
    valid: bool = false;
    passed: bool = false;
    weighted_score_bps: u32 = 0;
    pass_threshold_bps: u32 = 0;
    hard_gate_failed: bool = false;
    hard_gate_fail_count: usize = 0;
    failed_gate_index: usize = 0;
    below_min_count: usize = 0;
    invalid_dim_index: usize = 0;
    dimensions_evaluated: usize = 0;
}

/// Raw verdict from a single LLM judge call comparing `(first, second)`.
pub enum PairwiseRawVerdict {
    prefer_first,
    prefer_second,
    tie,
}

/// Bias-checked merged verdict after evaluating both `(A, B)` and swapped `(B, A)`.
pub enum PairwiseResolvedVerdict {
    win_a,
    win_b,
    tie,
    inconsistent_bias,
}

/// Inline candidate model record inside `JudgeArena`.
pub struct ArenaModelEntry {
    active: bool = false;
    model_id: u32 = 0;
    name_buf: [JUDGE_NAME_MAX_LEN]u8 = [0; JUDGE_NAME_MAX_LEN];
    name_len: usize = 0;
    elo: f64 = JUDGE_DEFAULT_ELO;
    wins: u64 = 0;
    losses: u64 = 0;
    ties: u64 = 0;

    pub fn nameSlice(): []const u8 {
        return name_buf[0..name_len];
    }

    pub fn matchesPlayed(): u64 {
        return wins + losses + ties;
    }

    /// Returns effective win rate in basis points (`0..=10000`, counting each tie as 0.5 win).
    pub fn winRateBps(): u32 {
        const played = matchesPlayed();
        if (played == 0) return 0;
        const num: u64 = (wins * 2 + ties) * (JUDGE_BPS_SCALE as u64);
        const den: u64 = played * 2;
        const bps: u64 = num / den;
        if (bps > (JUDGE_BPS_SCALE as u64)) return JUDGE_BPS_SCALE;
        return bps as u32;
    }
}

/// Sorted leaderboard entry returned by `JudgeArena.leaderboard`.
pub struct ArenaLeaderboardRow {
    slot_index: usize = 0;
    model_id: u32 = 0;
    elo: f64 = JUDGE_DEFAULT_ELO;
    wins: u64 = 0;
    losses: u64 = 0;
    ties: u64 = 0;
    win_rate_bps: u32 = 0;
}

/// Cumulative telemetry snapshot for `JudgeArena`.
pub struct JudgeArenaStats {
    model_count: usize = 0;
    capacity: usize = JUDGE_MAX_MODELS;
    total_comparisons: u64 = 0;
    valid_matches: u64 = 0;
    inconsistent_bias_count: u64 = 0;
    bias_rate_bps: u32 = 0;
    tie_count: u64 = 0;
}

// ============================================================================
// 2. Internal Helpers
// ============================================================================

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isValidLabel(s: []const u8): bool {
    if (s.len == 0 || s.len > JUDGE_NAME_MAX_LEN) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < 0x20 || s[i] == 0x7F) return false;
    }
    return true;
}

fn isFinitePositiveF64(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v) && v > 0.0;
}

fn appendRaw(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.* + i] = s[i];
    }
    pos.* = pos.* + s.len;
    return true;
}

fn appendU64(out: *[]u8, pos: *usize, val_in: u64): bool {
    if (val_in == 0) return appendRaw(out, pos, "0");
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val_in;
    while (v > 0) {
        tmp[n] = (0x30 as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* = pos.* + n;
    return true;
}

// ============================================================================
// 3. Multi-Dimensional Weighted Rubric (`JudgeRubric`)
// ============================================================================

/// Zero-heap multi-dimensional LLM evaluation rubric supporting up to 16 weighted dimensions,
/// per-dimension minimum thresholds, and hard-gate veto enforcement.
pub struct JudgeRubric {
    dims: [JUDGE_MAX_DIMENSIONS]JudgeDimensionSpec = [JudgeDimensionSpec{}; JUDGE_MAX_DIMENSIONS];
    dim_count: usize = 0;
    pass_threshold_bps: u32 = 7000;

    /// Initializes a rubric with a passing threshold in basis points (`0..=10000`).
    pub static fn init(pass_threshold_bps: u32): JudgeRubric? {
        if (pass_threshold_bps > JUDGE_BPS_SCALE) return null;
        return JudgeRubric{
            pass_threshold_bps: pass_threshold_bps,
        };
    }

    /// Registers a scoring dimension. Rejects invalid/duplicate names, `weight == 0`,
    /// `max_score == 0`, `min_score > max_score`, or full capacity (> 16 dimensions).
    pub fn addDimension(
        *_,
        name: []const u8,
        weight: u32,
        max_score: u32,
        min_score: u32,
        hard_gate: bool,
    ): usize? {
        if (dim_count >= JUDGE_MAX_DIMENSIONS) return null;
        if (!isValidLabel(name)) return null;
        if (weight == 0 || max_score == 0 || min_score > max_score) return null;

        let i: usize = 0;
        while (i < dim_count) : (i += 1) {
            if (bytesEq(dims[i].nameSlice(), name)) return null;
        }

        const idx = dim_count;
        let spec = JudgeDimensionSpec{};
        spec.active = true;
        spec.name_len = name.len;
        let k: usize = 0;
        while (k < name.len) : (k += 1) {
            spec.name_buf[k] = name[k];
        }
        spec.weight = weight;
        spec.max_score = max_score;
        spec.min_score = min_score;
        spec.hard_gate = hard_gate;

        dims[idx] = spec;
        dim_count += 1;
        return idx;
    }

    pub fn count(): usize {
        return dim_count;
    }

    pub fn totalWeight(): u64 {
        let sum: u64 = 0;
        let i: usize = 0;
        while (i < dim_count) : (i += 1) {
            sum += dims[i].weight as u64;
        }
        return sum;
    }

    pub fn getDimension(idx: usize): JudgeDimensionSpec? {
        if (idx >= dim_count) return null;
        return dims[idx];
    }

    /// Evaluates a score vector aligned with registered dimensions.
    /// Computes the weighted normalized score in `0..=10000` bps, checks every dimension's
    /// `min_score` and `hard_gate` veto, and guards against mismatched lengths or out-of-range scores.
    pub fn evaluate(scores: []const u32): RubricEvalReport {
        let rep = RubricEvalReport{
            pass_threshold_bps: pass_threshold_bps,
        };

        if (dim_count == 0 || scores.len != dim_count) {
            rep.valid = false;
            rep.passed = false;
            return rep;
        }

        let weighted_bps_sum: u64 = 0;
        let weight_sum: u64 = 0;

        let i: usize = 0;
        while (i < dim_count) : (i += 1) {
            const d = dims[i];
            const sc = scores[i];
            if (sc > d.max_score) {
                rep.valid = false;
                rep.passed = false;
                rep.invalid_dim_index = i;
                return rep;
            }

            // Normalized per-dimension bps: (sc * 10000) / max_score
            const dim_bps: u64 = ((sc as u64) * (JUDGE_BPS_SCALE as u64)) / (d.max_score as u64);
            weighted_bps_sum += dim_bps * (d.weight as u64);
            weight_sum += d.weight as u64;

            if (sc < d.min_score) {
                rep.below_min_count += 1;
                if (d.hard_gate) {
                    if (!rep.hard_gate_failed) {
                        rep.failed_gate_index = i;
                    }
                    rep.hard_gate_failed = true;
                    rep.hard_gate_fail_count += 1;
                }
            }
        }

        if (weight_sum == 0) {
            rep.valid = false;
            rep.passed = false;
            return rep;
        }

        let total_bps: u64 = weighted_bps_sum / weight_sum;
        if (total_bps > (JUDGE_BPS_SCALE as u64)) {
            total_bps = JUDGE_BPS_SCALE as u64;
        }

        rep.valid = true;
        rep.weighted_score_bps = total_bps as u32;
        rep.dimensions_evaluated = dim_count;
        rep.passed = !rep.hard_gate_failed && rep.weighted_score_bps >= pass_threshold_bps;
        return rep;
    }
}

// ============================================================================
// 4. Anti-Position-Bias Pairwise Verdict Resolver
// ============================================================================

/// Merges a forward pairwise judge verdict for `(A, B)` with a swapped-order verdict for `(B, A)`:
///   - `(prefer_first, prefer_second)` -> `win_a` (A won in both positions)
///   - `(prefer_second, prefer_first)` -> `win_b` (B won in both positions)
///   - `(tie, tie)`                    -> `tie`
///   - Any contradictory pair          -> `inconsistent_bias` (e.g. primacy/recency position bias)
pub fn resolvePairwiseBias(
    forward_verdict: PairwiseRawVerdict,
    swapped_verdict: PairwiseRawVerdict,
): PairwiseResolvedVerdict {
    if (forward_verdict == PairwiseRawVerdict.prefer_first && swapped_verdict == PairwiseRawVerdict.prefer_second) {
        return PairwiseResolvedVerdict.win_a;
    }
    if (forward_verdict == PairwiseRawVerdict.prefer_second && swapped_verdict == PairwiseRawVerdict.prefer_first) {
        return PairwiseResolvedVerdict.win_b;
    }
    if (forward_verdict == PairwiseRawVerdict.tie && swapped_verdict == PairwiseRawVerdict.tie) {
        return PairwiseResolvedVerdict.tie;
    }
    return PairwiseResolvedVerdict.inconsistent_bias;
}

/// Returns `true` when the judge exhibited pure positional bias (always picking slot 1 or always slot 2).
pub fn isPurePositionBias(
    forward_verdict: PairwiseRawVerdict,
    swapped_verdict: PairwiseRawVerdict,
): bool {
    return (forward_verdict == PairwiseRawVerdict.prefer_first && swapped_verdict == PairwiseRawVerdict.prefer_first) ||
        (forward_verdict == PairwiseRawVerdict.prefer_second && swapped_verdict == PairwiseRawVerdict.prefer_second);
}

// ============================================================================
// 5. Zero-Heap Multi-Model Elo Arena (`JudgeArena`)
// ============================================================================

/// Zero-heap Elo rating tracker for up to 16 candidate LLM models with automatic
/// filtering of `inconsistent_bias` pairwise comparisons.
pub struct JudgeArena {
    models: [JUDGE_MAX_MODELS]ArenaModelEntry = [ArenaModelEntry{}; JUDGE_MAX_MODELS];
    model_count: usize = 0;
    k_factor: f64 = JUDGE_DEFAULT_K_FACTOR;
    total_comparisons: u64 = 0;
    valid_matches: u64 = 0;
    inconsistent_bias_count: u64 = 0;
    tie_count: u64 = 0;

    /// Initializes a new arena with Elo `k_factor` in `(0.0, 256.0]`.
    pub static fn init(k_factor: f64): JudgeArena? {
        if (!isFinitePositiveF64(k_factor) || k_factor > 256.0) return null;
        return JudgeArena{
            k_factor: k_factor,
        };
    }

    /// Registers a candidate model with a non-zero `model_id`, display `name`, and positive `initial_elo`.
    pub fn registerModel(*_, model_id: u32, name: []const u8, initial_elo: f64): usize? {
        if (model_id == 0 || model_count >= JUDGE_MAX_MODELS) return null;
        if (!isValidLabel(name) || !isFinitePositiveF64(initial_elo)) return null;
        if (findModel(model_id) != null) return null;

        const slot = model_count;
        let entry = ArenaModelEntry{};
        entry.active = true;
        entry.model_id = model_id;
        entry.name_len = name.len;
        let k: usize = 0;
        while (k < name.len) : (k += 1) {
            entry.name_buf[k] = name[k];
        }
        entry.elo = initial_elo;

        models[slot] = entry;
        model_count += 1;
        return slot;
    }

    pub fn findModel(model_id: u32): usize? {
        let i: usize = 0;
        while (i < model_count) : (i += 1) {
            if (models[i].active && models[i].model_id == model_id) return i;
        }
        return null;
    }

    pub fn getModel(slot: usize): ArenaModelEntry? {
        if (slot >= model_count || !models[slot].active) return null;
        return models[slot];
    }

    pub fn modelCount(): usize {
        return model_count;
    }

    fn applyResolved(*_, slot_a: usize, slot_b: usize, verdict: PairwiseResolvedVerdict) {
        total_comparisons += 1;
        if (verdict == PairwiseResolvedVerdict.inconsistent_bias) {
            inconsistent_bias_count += 1;
            return;
        }

        const ra = models[slot_a].elo;
        const rb = models[slot_b].elo;
        const expected_a = 1.0 / (1.0 + std.math.pow(f64, 10.0, (rb - ra) / 400.0));
        const expected_b = 1.0 - expected_a;

        let score_a: f64 = 0.5;
        let score_b: f64 = 0.5;
        if (verdict == PairwiseResolvedVerdict.win_a) {
            score_a = 1.0;
            score_b = 0.0;
            models[slot_a].wins += 1;
            models[slot_b].losses += 1;
        } else if (verdict == PairwiseResolvedVerdict.win_b) {
            score_a = 0.0;
            score_b = 1.0;
            models[slot_a].losses += 1;
            models[slot_b].wins += 1;
        } else {
            models[slot_a].ties += 1;
            models[slot_b].ties += 1;
            tie_count += 1;
        }

        models[slot_a].elo = ra + k_factor * (score_a - expected_a);
        models[slot_b].elo = rb + k_factor * (score_b - expected_b);
        valid_matches += 1;
    }

    /// Resolves `(forward_verdict, swapped_verdict)` for `(model_a_id, model_b_id)` and updates Elo.
    /// Automatically ignores `inconsistent_bias` dirty samples without mutating Elo ratings.
    /// Returns the resolved verdict, or `null` if either model is unknown or IDs are identical.
    pub fn recordPairwiseMatch(
        *_,
        model_a_id: u32,
        model_b_id: u32,
        forward_verdict: PairwiseRawVerdict,
        swapped_verdict: PairwiseRawVerdict,
    ): PairwiseResolvedVerdict? {
        if (model_a_id == model_b_id) return null;
        const slot_a = findModel(model_a_id) ?? return null;
        const slot_b = findModel(model_b_id) ?? return null;

        const verdict = resolvePairwiseBias(forward_verdict, swapped_verdict);
        applyResolved(slot_a, slot_b, verdict);
        return verdict;
    }

    /// Records a pre-resolved pairwise verdict (still ignoring `inconsistent_bias` for Elo).
    pub fn recordResolvedMatch(
        *_,
        model_a_id: u32,
        model_b_id: u32,
        verdict: PairwiseResolvedVerdict,
    ): bool {
        if (model_a_id == model_b_id) return false;
        const slot_a = findModel(model_a_id) ?? return false;
        const slot_b = findModel(model_b_id) ?? return false;
        applyResolved(slot_a, slot_b, verdict);
        return true;
    }

    /// Writes a leaderboard sorted by descending Elo (tie-breaking by descending `wins`, then
    /// ascending `model_id`) into `out_rows`. Returns the number of rows written.
    pub fn leaderboard(out_rows: *[]ArenaLeaderboardRow): usize {
        if (model_count == 0 || out_rows.len == 0) return 0;

        let tmp: [JUDGE_MAX_MODELS]ArenaLeaderboardRow = [ArenaLeaderboardRow{}; JUDGE_MAX_MODELS];
        let i: usize = 0;
        while (i < model_count) : (i += 1) {
            const m = models[i];
            tmp[i] = ArenaLeaderboardRow{
                slot_index: i,
                model_id: m.model_id,
                elo: m.elo,
                wins: m.wins,
                losses: m.losses,
                ties: m.ties,
                win_rate_bps: m.winRateBps(),
            };
        }

        // Insertion sort (at most 16 models -> zero stack overhead, deterministic)
        let j: usize = 1;
        while (j < model_count) : (j += 1) {
            const key = tmp[j];
            let k: usize = j;
            while (k > 0) {
                const prev = tmp[k - 1];
                const swap_needed = (key.elo > prev.elo) ||
                    (key.elo == prev.elo && key.wins > prev.wins) ||
                    (key.elo == prev.elo && key.wins == prev.wins && key.model_id < prev.model_id);
                if (!swap_needed) break;
                tmp[k] = prev;
                k -= 1;
            }
            tmp[k] = key;
        }

        const n_out: usize = if (model_count < out_rows.len) model_count else out_rows.len;
        let w: usize = 0;
        while (w < n_out) : (w += 1) {
            out_rows[w] = tmp[w];
        }
        return n_out;
    }

    /// Returns a point-in-time telemetry snapshot including position-bias rate in bps.
    pub fn stats(): JudgeArenaStats {
        let bias_bps: u32 = 0;
        if (total_comparisons > 0) {
            const raw_bps: u64 = (inconsistent_bias_count * (JUDGE_BPS_SCALE as u64)) / total_comparisons;
            bias_bps = if (raw_bps > (JUDGE_BPS_SCALE as u64)) JUDGE_BPS_SCALE else (raw_bps as u32);
        }
        return JudgeArenaStats{
            model_count: model_count,
            capacity: JUDGE_MAX_MODELS,
            total_comparisons: total_comparisons,
            valid_matches: valid_matches,
            inconsistent_bias_count: inconsistent_bias_count,
            bias_rate_bps: bias_bps,
            tie_count: tie_count,
        };
    }
}

// ============================================================================
// 6. Zero-Alloc Release-Gate Summary Formatter
// ============================================================================

/// Formats a compact JSON release-gate summary combining rubric evaluation and arena bias telemetry
/// into `out_buf`. Gate passes iff `eval_report.passed && arena_stats.bias_rate_bps <= max_bias_bps`.
/// Returns bytes written, or `null` if `out_buf` is too small.
pub fn formatReleaseGateSummary(
    eval_report: RubricEvalReport,
    arena_stats: JudgeArenaStats,
    max_bias_bps: u32,
    out_buf: *[]u8,
): usize? {
    const gate_pass = eval_report.valid && eval_report.passed && arena_stats.bias_rate_bps <= max_bias_bps;
    let pos: usize = 0;

    if (!appendRaw(out_buf, *pos, "{\"gate\":\"")) return null;
    if (!appendRaw(out_buf, *pos, if (gate_pass) "PASS" else "FAIL")) return null;
    if (!appendRaw(out_buf, *pos, "\",\"scoreBps\":")) return null;
    if (!appendU64(out_buf, *pos, eval_report.weighted_score_bps as u64)) return null;
    if (!appendRaw(out_buf, *pos, ",\"hardGateFailed\":")) return null;
    if (!appendRaw(out_buf, *pos, if (eval_report.hard_gate_failed) "true" else "false")) return null;
    if (!appendRaw(out_buf, *pos, ",\"validMatches\":")) return null;
    if (!appendU64(out_buf, *pos, arena_stats.valid_matches)) return null;
    if (!appendRaw(out_buf, *pos, ",\"biasCount\":")) return null;
    if (!appendU64(out_buf, *pos, arena_stats.inconsistent_bias_count)) return null;
    if (!appendRaw(out_buf, *pos, ",\"biasRateBps\":")) return null;
    if (!appendU64(out_buf, *pos, arena_stats.bias_rate_bps as u64)) return null;
    if (!appendRaw(out_buf, *pos, "}")) return null;
    return pos;
}
