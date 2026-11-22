// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/episodic.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation AI Agent episodic memory:
// Park et al. (2023) 3-factor scoring, Ebbinghaus retention & spaced-repetition reinforcement,
// deterministic Top-K ranking, pinned-safe eviction, and defensive input guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MemoryWeights,
    EpisodeMeta,
    ScoredEpisode,
    ebbinghausRetention,
    reinforceStability,
    scoreEpisode,
    rankTopEpisodes,
    selectEvictionVictim,
    touchEpisode,
} from "cap:agentmem";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff: f64 = if (a >= b) a - b else b - a;
    return diff <= eps;
}

fn testEbbinghausDecayAndReinforcement()! {
    // 1. Exact curve checkpoints: t=0 -> 1.0, t=S -> e^-1 (~0.367879), t=2S -> e^-2 (~0.135335)
    expect_eq(approxEq(ebbinghausRetention(0.0, 20.0), 1.0, 0.000000000001), true);
    expect_eq(approxEq(ebbinghausRetention(-5.0, 20.0), 1.0, 0.000000000001), true);
    expect_eq(approxEq(ebbinghausRetention(20.0, 20.0), std.math.exp(-1.0), 0.000000000001), true);
    expect_eq(approxEq(ebbinghausRetention(40.0, 20.0), std.math.exp(-2.0), 0.000000000001), true);

    // 2. Strictly monotonic decay over time
    const r1 = ebbinghausRetention(1.0, 10.0);
    const r5 = ebbinghausRetention(5.0, 10.0);
    const r24 = ebbinghausRetention(24.0, 10.0);
    expect_eq(r1 > r5, true);
    expect_eq(r5 > r24, true);
    expect_eq(r24 > 0.0, true);

    // 3. Spaced-repetition reinforcement increases stability with diminishing marginal gain
    const s0: f64 = 1.0;
    const s1 = reinforceStability(s0, 0);
    const s2 = reinforceStability(s1, 1);
    const s3 = reinforceStability(s2, 10);
    expect_eq(s1 > s0, true);
    expect_eq(s2 > s1, true);
    expect_eq(s3 > s2, true);
    // Higher stability yields higher retention at the same elapsed time
    expect_eq(ebbinghausRetention(12.0, s2 * 10.0) > ebbinghausRetention(12.0, s0 * 10.0), true);

    // 4. Defensive edge cases: NaN, Inf, non-positive stability
    const nan_val: f64 = std.math.nan(f64);
    const inf_val: f64 = std.math.inf(f64);
    expect_eq(ebbinghausRetention(nan_val, 10.0), 0.0);
    expect_eq(ebbinghausRetention(5.0, nan_val), 0.0);
    expect_eq(ebbinghausRetention(5.0, 0.0), 0.0);
    expect_eq(ebbinghausRetention(5.0, -2.0), 0.0);
    expect_eq(ebbinghausRetention(inf_val, 10.0), 0.0);
    expect_eq(ebbinghausRetention(5.0, inf_val), 1.0);
}

fn testPark2023ThreeFactorTradeoff()! {
    const now_sec: i64 = 1000000;
    const hour: i64 = 3600;

    // Episode A: Very recent (1h ago), high importance (0.9), low relevance (0.2)
    const ep_recent_imp = EpisodeMeta{
        id: 1,
        created_at_sec: now_sec - hour,
        last_accessed_sec: now_sec - hour,
        access_count: 1,
        importance: 0.9,
        strength: 1.0,
        pinned: false,
    };

    // Episode B: Older (24h ago), moderate importance (0.5), very high relevance (0.98)
    const ep_old_rel = EpisodeMeta{
        id: 2,
        created_at_sec: now_sec - 24 * hour,
        last_accessed_sec: now_sec - 24 * hour,
        access_count: 1,
        importance: 0.5,
        strength: 1.0,
        pinned: false,
    };

    // Under default balanced weights (1, 1, 1, decay=0.05), recent+important wins over 24h-old relevant
    const w_def = MemoryWeights.default();
    const sa_def_opt = scoreEpisode(ep_recent_imp, now_sec, 0.2, w_def);
    const sb_def_opt = scoreEpisode(ep_old_rel, now_sec, 0.98, w_def);
    if (sa := sa_def_opt) {
        if (sb := sb_def_opt) {
            expect_eq(sa.recency_score > sb.recency_score, true);
            expect_eq(sa.importance_score, 0.9);
            expect_eq(sb.relevance_score, 0.98);
            expect_eq(sa.total_score > sb.total_score, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Under relevance-dominant weights (0.5, 0.5, 4.0), old highly-relevant memory decisively wins
    const w_rel = MemoryWeights{ recency: 0.5, importance: 0.5, relevance: 4.0, decay_rate: 0.05 };
    if (sa := scoreEpisode(ep_recent_imp, now_sec, 0.2, w_rel)) {
        if (sb := scoreEpisode(ep_old_rel, now_sec, 0.98, w_rel)) {
            expect_eq(sb.total_score > sa.total_score, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testRankTopEpisodesDeterministicTieBreak()! {
    const now_sec: i64 = 500000;
    const hour: i64 = 3600;
    const w = MemoryWeights.default();

    // Two episodes (id=30 and id=10) have identical attributes & relevance -> tie must break by id ascending (10 before 30)
    const eps: [4]EpisodeMeta = [
        EpisodeMeta{ id: 30, created_at_sec: now_sec - 2 * hour, last_accessed_sec: now_sec - 2 * hour, access_count: 1, importance: 0.7, strength: 1.0, pinned: false },
        EpisodeMeta{ id: 20, created_at_sec: now_sec - 48 * hour, last_accessed_sec: now_sec - 48 * hour, access_count: 0, importance: 0.2, strength: 1.0, pinned: false },
        EpisodeMeta{ id: 10, created_at_sec: now_sec - 2 * hour, last_accessed_sec: now_sec - 2 * hour, access_count: 1, importance: 0.7, strength: 1.0, pinned: false },
        EpisodeMeta{ id: 40, created_at_sec: now_sec, last_accessed_sec: now_sec, access_count: 2, importance: 0.95, strength: 1.5, pinned: true },
    ];
    const rels: [4]f64 = [0.8, 0.1, 0.8, 0.95];

    let out_arr: [3]ScoredEpisode = undefined;
    let out_slice: []ScoredEpisode = out_arr[0..];

    const n_opt = rankTopEpisodes(eps[0..], rels[0..], now_sec, w, 3, out_slice);
    if (n := n_opt) {
        expect_eq(n, 3);
        expect_eq(out_slice[0].id, 40);
        expect_eq(out_slice[1].id, 10);
        expect_eq(out_slice[2].id, 30);
        expect_eq(out_slice[0].total_score > out_slice[1].total_score, true);
        expect_eq(approxEq(out_slice[1].total_score, out_slice[2].total_score, 0.000000000001), true);
    } else {
        expect_eq(true, false);
    }

    // top_k == 0 returns 0 even with empty output buffer
    let zero_slice: []ScoredEpisode = out_arr[0..0];
    if (n0 := rankTopEpisodes(eps[0..], rels[0..], now_sec, w, 0, zero_slice)) {
        expect_eq(n0, 0);
    } else {
        expect_eq(true, false);
    }
}

fn testSelectEvictionVictimAndPinnedProtection()! {
    const now_sec: i64 = 900000;
    const hour: i64 = 3600;
    const w = MemoryWeights.default();

    // Slot 0: very stale & low importance, BUT pinned -> MUST NOT be evicted!
    // Slot 1: recent & high importance -> keep
    // Slot 2: stale (30h) & low importance (0.15) -> EVICTION VICTIM (index 2)
    // Slot 3: medium age (5h) & medium importance (0.6) -> keep
    const eps: [4]EpisodeMeta = [
        EpisodeMeta{ id: 1, created_at_sec: now_sec - 200 * hour, last_accessed_sec: now_sec - 200 * hour, access_count: 0, importance: 0.01, strength: 1.0, pinned: true },
        EpisodeMeta{ id: 2, created_at_sec: now_sec - hour, last_accessed_sec: now_sec - hour, access_count: 3, importance: 0.9, strength: 1.5, pinned: false },
        EpisodeMeta{ id: 3, created_at_sec: now_sec - 30 * hour, last_accessed_sec: now_sec - 30 * hour, access_count: 0, importance: 0.15, strength: 1.0, pinned: false },
        EpisodeMeta{ id: 4, created_at_sec: now_sec - 5 * hour, last_accessed_sec: now_sec - 5 * hour, access_count: 1, importance: 0.6, strength: 1.0, pinned: false },
    ];

    if (victim := selectEvictionVictim(eps[0..], now_sec, w)) {
        expect_eq(victim, 2);
    } else {
        expect_eq(true, false);
    }

    // All pinned -> returns null
    const all_pinned: [2]EpisodeMeta = [
        EpisodeMeta{ id: 10, created_at_sec: 0, last_accessed_sec: 0, access_count: 0, importance: 0.1, strength: 1.0, pinned: true },
        EpisodeMeta{ id: 11, created_at_sec: 0, last_accessed_sec: 0, access_count: 0, importance: 0.2, strength: 1.0, pinned: true },
    ];
    expect_eq(selectEvictionVictim(all_pinned[0..], now_sec, w) == null, true);

    // Empty slice -> returns null
    const empty_eps: []const EpisodeMeta = [];
    expect_eq(selectEvictionVictim(empty_eps, now_sec, w) == null, true);
}

fn testTouchEpisodeRecallReinforcement()! {
    const t0: i64 = 100000;
    const hour: i64 = 3600;
    const w = MemoryWeights.default();

    let ep_untouched = EpisodeMeta{
        id: 101,
        created_at_sec: t0,
        last_accessed_sec: t0,
        access_count: 0,
        importance: 0.6,
        strength: 1.0,
        pinned: false,
    };
    let ep_touched = ep_untouched;
    ep_touched.id = 102;

    // Recall ep_touched at t0 + 2h
    touchEpisode(*ep_touched, t0 + 2 * hour);
    expect_eq(ep_touched.last_accessed_sec, t0 + 2 * hour);
    expect_eq(ep_touched.access_count, 1);
    expect_eq(ep_touched.strength > 1.0, true);

    // Evaluate both 10 hours AFTER their respective last access:
    // Because ep_touched has higher strength, its Ebbinghaus retention & total score are strictly higher!
    const score_a_opt = scoreEpisode(ep_untouched, t0 + 10 * hour, 0.7, w);
    const score_b_opt = scoreEpisode(ep_touched, t0 + 12 * hour, 0.7, w);
    if (sa := score_a_opt) {
        if (sb := score_b_opt) {
            expect_eq(sb.recency_score > sa.recency_score, true);
            expect_eq(sb.total_score > sa.total_score, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Touching with an older timestamp does not rewind last_accessed_sec
    touchEpisode(*ep_touched, t0);
    expect_eq(ep_touched.last_accessed_sec, t0 + 2 * hour);
    expect_eq(ep_touched.access_count, 2);
}

fn testDefensiveGuardsAndCapNamespace()! {
    const now_sec: i64 = 200000;
    const nan_val: f64 = std.math.nan(f64);
    const w_ok = cap.agentmem.MemoryWeights.default();

    const valid_ep = EpisodeMeta{
        id: 1,
        created_at_sec: now_sec,
        last_accessed_sec: now_sec,
        access_count: 0,
        importance: 0.5,
        strength: 1.0,
        pinned: false,
    };

    // 1. Invalid weights (all zero, negative, NaN)
    const w_zero = MemoryWeights{ recency: 0.0, importance: 0.0, relevance: 0.0, decay_rate: 0.05 };
    const w_neg = MemoryWeights{ recency: -1.0, importance: 1.0, relevance: 1.0, decay_rate: 0.05 };
    const w_bad_decay = MemoryWeights{ recency: 1.0, importance: 1.0, relevance: 1.0, decay_rate: -0.1 };
    const w_nan = MemoryWeights{ recency: nan_val, importance: 1.0, relevance: 1.0, decay_rate: 0.05 };

    expect_eq(scoreEpisode(valid_ep, now_sec, 0.5, w_zero) == null, true);
    expect_eq(scoreEpisode(valid_ep, now_sec, 0.5, w_neg) == null, true);
    expect_eq(scoreEpisode(valid_ep, now_sec, 0.5, w_bad_decay) == null, true);
    expect_eq(scoreEpisode(valid_ep, now_sec, 0.5, w_nan) == null, true);

    // 2. Out-of-range importance / relevance / strength / NaN
    let bad_imp = valid_ep;
    bad_imp.importance = 1.5;
    expect_eq(scoreEpisode(bad_imp, now_sec, 0.5, w_ok) == null, true);

    bad_imp.importance = -0.1;
    expect_eq(scoreEpisode(bad_imp, now_sec, 0.5, w_ok) == null, true);

    bad_imp.importance = nan_val;
    expect_eq(scoreEpisode(bad_imp, now_sec, 0.5, w_ok) == null, true);

    let bad_str = valid_ep;
    bad_str.strength = 0.0;
    expect_eq(scoreEpisode(bad_str, now_sec, 0.5, w_ok) == null, true);

    expect_eq(scoreEpisode(valid_ep, now_sec, 1.2, w_ok) == null, true);
    expect_eq(scoreEpisode(valid_ep, now_sec, nan_val, w_ok) == null, true);

    // 3. rankTopEpisodes: mismatched lengths & insufficient out_scored buffer
    const eps: [2]EpisodeMeta = [valid_ep, valid_ep];
    const rels_short: [1]f64 = [0.5];
    const rels_ok: [2]f64 = [0.5, 0.8];
    let small_out: [1]ScoredEpisode = undefined;
    let small_slice: []ScoredEpisode = small_out[0..];

    expect_eq(rankTopEpisodes(eps[0..], rels_short[0..], now_sec, w_ok, 1, small_slice) == null, true);
    expect_eq(rankTopEpisodes(eps[0..], rels_ok[0..], now_sec, w_ok, 2, small_slice) == null, true);

    // 4. Empty slices return 0 cleanly
    const empty_eps: []const EpisodeMeta = [];
    const empty_rels: []const f64 = [];
    if (n := cap.agentmem.rankTopEpisodes(empty_eps, empty_rels, now_sec, w_ok, 5, small_slice)) {
        expect_eq(n, 0);
    } else {
        expect_eq(true, false);
    }
}

fn main() {
    test("1. episodic: Ebbinghaus retention decay curve & spaced-repetition reinforcement", () => {
        testEbbinghausDecayAndReinforcement()?;
    });

    test("2. episodic: Park 2023 3-factor scoring tradeoff (recency+importance vs relevance)", () => {
        testPark2023ThreeFactorTradeoff()?;
    });

    test("3. episodic: rankTopEpisodes deterministic Top-K ranking & ascending id tie-break", () => {
        testRankTopEpisodesDeterministicTieBreak()?;
    });

    test("4. episodic: selectEvictionVictim protects pinned slots and evicts lowest utility", () => {
        testSelectEvictionVictimAndPinnedProtection()?;
    });

    test("5. episodic: touchEpisode recall reinforcement boosts subsequent retention & score", () => {
        testTouchEpisodeRecallReinforcement()?;
    });

    test("6. episodic: defensive guards (invalid weights, out-of-range/NaN, small buffer) & cap.agentmem", () => {
        testDefensiveGuardsAndCapNamespace()?;
    });
}
