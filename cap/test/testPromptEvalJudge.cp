// Origin: Pure-CP Stdlib Test Suite (lib/prompt/evalJudge.cp) | Created: 2026-11-23
// Production Deep Verification Suite for LLM-as-a-Judge Rubric & Anti-Position-Bias Pairwise Arena:
//   1. Multi-dimensional weighted Rubric evaluation (0..10000 bps normalization) & pass threshold
//   2. Hard-gate veto enforcement, dimension bounds validation & 16-dimension capacity saturation
//   3. Complete 3x3 truth table for `resolvePairwiseBias` & `isPurePositionBias`
//   4. Multi-model `JudgeArena` Elo rating updates, tie handling & automatic `inconsistent_bias` filtering
//   5. Deterministic sorted leaderboard, win-rate bps & 16-model capacity/validation guards
//   6. Zero-alloc JSON release-gate summary formatter & `cap.prompt` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    JUDGE_MAX_DIMENSIONS,
    JUDGE_MAX_MODELS,
    JUDGE_BPS_SCALE,
    JUDGE_DEFAULT_ELO,
    JUDGE_DEFAULT_K_FACTOR,
    JudgeDimensionSpec,
    RubricEvalReport,
    PairwiseRawVerdict,
    PairwiseResolvedVerdict,
    ArenaModelEntry,
    ArenaLeaderboardRow,
    JudgeArenaStats,
    JudgeRubric,
    resolvePairwiseBias,
    isPurePositionBias,
    JudgeArena,
    formatReleaseGateSummary,
} from "cap:prompt";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("EvalJudge: multi-dimensional weighted Rubric bps calculation & hard-gate veto") {
    expect_true(JudgeRubric.init(10001) == null);
    let rubric = JudgeRubric.init(7500) ?? return;

    // 3 dimensions: safety (weight 4, max 5, min 4, hard_gate=true), accuracy (weight 4, max 10, min 6), style (weight 2, max 5, min 2)
    expect_eq(rubric.addDimension("safety", 4, 5, 4, true) ?? 99, 0 as usize);
    expect_eq(rubric.addDimension("accuracy", 4, 10, 6, false) ?? 99, 1 as usize);
    expect_eq(rubric.addDimension("style", 2, 5, 2, false) ?? 99, 2 as usize);
    expect_eq(rubric.count(), 3 as usize);
    expect_eq(rubric.totalWeight(), 10 as u64);

    // Reject duplicate name, zero weight, zero max_score, min_score > max_score
    expect_true(rubric.addDimension("safety", 1, 5, 1, false) == null);
    expect_true(rubric.addDimension("zero_w", 0, 5, 1, false) == null);
    expect_true(rubric.addDimension("zero_max", 1, 0, 0, false) == null);
    expect_true(rubric.addDimension("bad_min", 1, 5, 6, false) == null);

    // Passing case: safety=5/5 (10000*4=40000), accuracy=8/10 (8000*4=32000), style=4/5 (8000*2=16000) -> 88000/10 = 8800 bps
    const pass_scores: [3]u32 = [5, 8, 4];
    const r_pass = rubric.evaluate(pass_scores[0..]);
    expect_true(r_pass.valid);
    expect_true(r_pass.passed);
    expect_eq(r_pass.weighted_score_bps, 8800 as u32);
    expect_eq(r_pass.hard_gate_failed, false);
    expect_eq(r_pass.below_min_count, 0 as usize);

    // Hard-gate veto even when weighted score exceeds pass_threshold_bps:
    // safety=3/5 (6000*4=24000, < min 4, hard_gate!), accuracy=10/10 (40000), style=5/5 (20000) -> 8400 bps >= 7500, but VETOED
    const veto_scores: [3]u32 = [3, 10, 5];
    const r_veto = rubric.evaluate(veto_scores[0..]);
    expect_true(r_veto.valid);
    expect_eq(r_veto.weighted_score_bps, 8400 as u32);
    expect_true(r_veto.hard_gate_failed);
    expect_eq(r_veto.failed_gate_index, 0 as usize);
    expect_eq(r_veto.hard_gate_fail_count, 1 as usize);
    expect_eq(r_veto.passed, false);

    // Score below threshold without hard-gate failure
    const low_scores: [3]u32 = [4, 5, 2]; // (8000*4 + 5000*4 + 4000*2)/10 = 6000 bps < 7500
    const r_low = rubric.evaluate(low_scores[0..]);
    expect_true(r_low.valid);
    expect_eq(r_low.weighted_score_bps, 6000 as u32);
    expect_eq(r_low.hard_gate_failed, false);
    expect_eq(r_low.below_min_count, 1 as usize); // accuracy 5 < 6
    expect_eq(r_low.passed, false);
}

test("EvalJudge: dimension bounds validation, mismatched length & 16-dimension saturation") {
    let rubric = JudgeRubric.init(5000) ?? return;
    let name_buf: [8]u8 = [0; 8];

    // Fill all 16 dimensions
    let i: usize = 0;
    while (i < JUDGE_MAX_DIMENSIONS) : (i += 1) {
        name_buf[0] = 0x64; // 'd'
        name_buf[1] = (0x30 as u8) + ((i / 10) as u8);
        name_buf[2] = (0x30 as u8) + ((i % 10) as u8);
        expect_true(rubric.addDimension(name_buf[0..3], 1, 10, 0, false) != null);
    }
    expect_eq(rubric.count(), JUDGE_MAX_DIMENSIONS);
    // 17th dimension rejected
    expect_true(rubric.addDimension("overflow", 1, 10, 0, false) == null);

    // Mismatched score slice length -> invalid
    const short_scores: [3]u32 = [10, 10, 10];
    expect_eq(rubric.evaluate(short_scores[0..]).valid, false);

    // Out-of-range score (> max_score) -> invalid with exact invalid_dim_index
    let bad_scores: [JUDGE_MAX_DIMENSIONS]u32 = [5; JUDGE_MAX_DIMENSIONS];
    bad_scores[11] = 11; // max_score is 10
    const r_bad = rubric.evaluate(bad_scores[0..]);
    expect_eq(r_bad.valid, false);
    expect_eq(r_bad.passed, false);
    expect_eq(r_bad.invalid_dim_index, 11 as usize);
}

test("EvalJudge: complete 3x3 truth table for resolvePairwiseBias & isPurePositionBias") {
    // Consistent outcomes
    expect_eq(
        resolvePairwiseBias(PairwiseRawVerdict.prefer_first, PairwiseRawVerdict.prefer_second),
        PairwiseResolvedVerdict.win_a,
    );
    expect_eq(
        resolvePairwiseBias(PairwiseRawVerdict.prefer_second, PairwiseRawVerdict.prefer_first),
        PairwiseResolvedVerdict.win_b,
    );
    expect_eq(
        resolvePairwiseBias(PairwiseRawVerdict.tie, PairwiseRawVerdict.tie),
        PairwiseResolvedVerdict.tie,
    );

    // Pure primacy / recency positional bias
    expect_eq(
        resolvePairwiseBias(PairwiseRawVerdict.prefer_first, PairwiseRawVerdict.prefer_first),
        PairwiseResolvedVerdict.inconsistent_bias,
    );
    expect_true(isPurePositionBias(PairwiseRawVerdict.prefer_first, PairwiseRawVerdict.prefer_first));

    expect_eq(
        resolvePairwiseBias(PairwiseRawVerdict.prefer_second, PairwiseRawVerdict.prefer_second),
        PairwiseResolvedVerdict.inconsistent_bias,
    );
    expect_true(isPurePositionBias(PairwiseRawVerdict.prefer_second, PairwiseRawVerdict.prefer_second));

    // Partial contradiction (win + tie) -> inconsistent_bias, but not pure position bias
    expect_eq(
        resolvePairwiseBias(PairwiseRawVerdict.prefer_first, PairwiseRawVerdict.tie),
        PairwiseResolvedVerdict.inconsistent_bias,
    );
    expect_eq(isPurePositionBias(PairwiseRawVerdict.prefer_first, PairwiseRawVerdict.tie), false);
    expect_eq(
        resolvePairwiseBias(PairwiseRawVerdict.tie, PairwiseRawVerdict.prefer_second),
        PairwiseResolvedVerdict.inconsistent_bias,
    );
}

test("EvalJudge: JudgeArena Elo updates, tie handling & inconsistent_bias dirty-sample filtering") {
    let arena = JudgeArena.init(JUDGE_DEFAULT_K_FACTOR) ?? return;
    const s1 = arena.registerModel(101, "model-alpha", JUDGE_DEFAULT_ELO) ?? return;
    const s2 = arena.registerModel(102, "model-beta", JUDGE_DEFAULT_ELO) ?? return;
    const s3 = arena.registerModel(103, "model-gamma", JUDGE_DEFAULT_ELO) ?? return;

    // 1. Alpha beats Beta consistently -> Alpha +16 (1516), Beta -16 (1484)
    const v1 = arena.recordPairwiseMatch(101, 102, PairwiseRawVerdict.prefer_first, PairwiseRawVerdict.prefer_second);
    expect_eq(v1 ?? PairwiseResolvedVerdict.tie, PairwiseResolvedVerdict.win_a);
    const m1_after = arena.getModel(s1) ?? ArenaModelEntry{};
    const m2_after = arena.getModel(s2) ?? ArenaModelEntry{};
    expect_true(m1_after.elo > 1515.9 && m1_after.elo < 1516.1);
    expect_true(m2_after.elo > 1483.9 && m2_after.elo < 1484.1);

    // 2. Position-biased dirty comparison between Alpha and Beta -> MUST NOT mutate Elo or W/L counts
    const v_bias = arena.recordPairwiseMatch(101, 102, PairwiseRawVerdict.prefer_first, PairwiseRawVerdict.prefer_first);
    expect_eq(v_bias ?? PairwiseResolvedVerdict.tie, PairwiseResolvedVerdict.inconsistent_bias);
    const m1_unchanged = arena.getModel(s1) ?? ArenaModelEntry{};
    expect_eq(m1_unchanged.elo, m1_after.elo);
    expect_eq(m1_unchanged.wins, 1 as u64);

    // 3. Beta vs Gamma tie -> Gamma (1500) drops slightly, Beta (1484) gains slightly
    const v_tie = arena.recordPairwiseMatch(102, 103, PairwiseRawVerdict.tie, PairwiseRawVerdict.tie);
    expect_eq(v_tie ?? PairwiseResolvedVerdict.win_a, PairwiseResolvedVerdict.tie);
    const m3_after = arena.getModel(s3) ?? ArenaModelEntry{};
    expect_eq(m3_after.ties, 1 as u64);
    expect_eq(m3_after.winRateBps(), 5000 as u32);

    // 4. Leaderboard sorted descending by Elo: Alpha > Gamma > Beta
    let rows: [4]ArenaLeaderboardRow = [ArenaLeaderboardRow{}; 4];
    let rows_s: []ArenaLeaderboardRow = rows[0..];
    const n_rows = arena.leaderboard(*rows_s);
    expect_eq(n_rows, 3 as usize);
    expect_eq(rows[0].model_id, 101 as u32);
    expect_eq(rows[1].model_id, 103 as u32);
    expect_eq(rows[2].model_id, 102 as u32);

    // Stats: 3 total comparisons, 2 valid matches, 1 inconsistent_bias (3333 bps), 1 tie
    const st = arena.stats();
    expect_eq(st.total_comparisons, 3 as u64);
    expect_eq(st.valid_matches, 2 as u64);
    expect_eq(st.inconsistent_bias_count, 1 as u64);
    expect_eq(st.bias_rate_bps, 3333 as u32);
    expect_eq(st.tie_count, 1 as u64);
}

test("EvalJudge: 16-model arena capacity saturation & invalid input guards") {
    expect_true(JudgeArena.init(0.0) == null);
    expect_true(JudgeArena.init(-10.0) == null);
    expect_true(JudgeArena.init(300.0) == null);

    let arena = JudgeArena.init(24.0) ?? return;
    expect_true(arena.registerModel(0, "zero-id", 1500.0) == null);
    expect_true(arena.registerModel(1, "", 1500.0) == null);
    expect_true(arena.registerModel(1, "neg-elo", -100.0) == null);

    let i: u32 = 1;
    while (i <= (JUDGE_MAX_MODELS as u32)) : (i += 1) {
        expect_true(arena.registerModel(i, "cand", 1500.0) != null);
    }
    expect_eq(arena.modelCount(), JUDGE_MAX_MODELS);
    // Duplicate ID and 17th model rejected
    expect_true(arena.registerModel(1, "dup", 1500.0) == null);
    expect_true(arena.registerModel(99, "overflow", 1500.0) == null);

    // Self-match & unknown model match rejected
    expect_true(arena.recordPairwiseMatch(1, 1, PairwiseRawVerdict.tie, PairwiseRawVerdict.tie) == null);
    expect_true(arena.recordPairwiseMatch(1, 999, PairwiseRawVerdict.tie, PairwiseRawVerdict.tie) == null);
    expect_eq(arena.recordResolvedMatch(1, 1, PairwiseResolvedVerdict.win_a), false);
}

test("EvalJudge: zero-alloc release-gate JSON summary & cap.prompt facade parity") {
    let rubric = cap.prompt.JudgeRubric.init(8000) ?? return;
    * = rubric.addDimension("quality", 1, 10, 7, true);
    const sc_ok: [1]u32 = [9];
    const eval_ok = rubric.evaluate(sc_ok[0..]);

    let arena = cap.prompt.JudgeArena.init(32.0) ?? return;
    * = arena.registerModel(1, "v1", 1500.0);
    * = arena.registerModel(2, "v2", 1500.0);
    * = arena.recordResolvedMatch(1, 2, PairwiseResolvedVerdict.win_a);
    * = arena.recordResolvedMatch(1, 2, PairwiseResolvedVerdict.win_a);
    * = arena.recordResolvedMatch(1, 2, PairwiseResolvedVerdict.win_b);
    * = arena.recordResolvedMatch(1, 2, PairwiseResolvedVerdict.inconsistent_bias); // 2500 bps bias

    const st = arena.stats();
    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];

    // max_bias_bps = 3000 -> PASS
    const n_pass = formatReleaseGateSummary(eval_ok, st, 3000, *buf_s) ?? 0;
    expect_true(n_pass > 0);
    expect_true(containsSubstr(buf[0..n_pass], "\"gate\":\"PASS\""));
    expect_true(containsSubstr(buf[0..n_pass], "\"scoreBps\":9000"));
    expect_true(containsSubstr(buf[0..n_pass], "\"hardGateFailed\":false"));
    expect_true(containsSubstr(buf[0..n_pass], "\"validMatches\":3"));
    expect_true(containsSubstr(buf[0..n_pass], "\"biasCount\":1"));
    expect_true(containsSubstr(buf[0..n_pass], "\"biasRateBps\":2500"));

    // Stricter max_bias_bps = 1000 -> FAIL due to judge position bias rate
    const n_fail = formatReleaseGateSummary(eval_ok, st, 1000, *buf_s) ?? 0;
    expect_true(containsSubstr(buf[0..n_fail], "\"gate\":\"FAIL\""));

    // Tiny buffer guard
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatReleaseGateSummary(eval_ok, st, 3000, *tiny_s) == null);
    expect_eq(JUDGE_BPS_SCALE, 10000 as u32);
}
