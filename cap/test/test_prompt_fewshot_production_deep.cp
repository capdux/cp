// Origin: Pure-CP Stdlib Test Suite (lib/prompt/fewshot.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation dynamic Few-Shot exemplar selector:
// case-insensitive bag-of-words Jaccard similarity (exact/partial/orthogonal), relevance-ranked
// selection under token budget & per-label class balance constraints, pairwise similarity
// deduplication, zero-allocation Few-Shot prompt block rendering, and defensive guards.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FewShotExample,
    FewShotSelection,
    FewShotConfig,
    tokenJaccardSimilarity,
    selectFewShotExamples,
    renderFewShotBlock,
} from "cap:prompt";

fn approxEqual(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return d >= -eps && d <= eps;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn testTokenJaccardSimilarityMetrics()! {
    let ha_arr: [32]u64 = undefined;
    let hb_arr: [32]u64 = undefined;
    let ha: []u64 = ha_arr[0..];
    let hb: []u64 = hb_arr[0..];

    // 1. Exact match (case-insensitive + punctuation & duplicate words ignored)
    const s_exact = tokenJaccardSimilarity("Hello, WORLD! world", "hello world", ha, hb);
    expect_eq(approxEqual(s_exact, 1.0, 1.0e-9), true);

    // 2. Partial overlap: {"fast", "cp", "compiler"} vs {"fast", "zig", "compiler", "native"}
    // intersection = 2 ("fast", "compiler"), union = 5 => 2/5 = 0.4
    const s_part = tokenJaccardSimilarity("Fast CP compiler", "fast zig compiler native", ha, hb);
    expect_eq(approxEqual(s_part, 0.4, 1.0e-9), true);

    // 3. Orthogonal texts => 0.0
    const s_zero = tokenJaccardSimilarity("apple banana", "quantum physics", ha, hb);
    expect_eq(s_zero, 0.0);

    // 4. Empty string => 0.0
    expect_eq(tokenJaccardSimilarity("", "hello", ha, hb), 0.0);
    expect_eq(tokenJaccardSimilarity("", "", ha, hb), 0.0);
}

fn testRelevanceBudgetAndClassBalance()! {
    // Query: "reset account password"
    // Pool:
    //  0: "reset account password now" (label 1, tokens 40) -> score 3/4 = 0.75 (highest)
    //  1: "reset account email"        (label 1, tokens 30) -> score 2/4 = 0.50 (2nd, same label 1)
    //  2: "password policy guide"      (label 2, tokens 50) -> score 1/5 = 0.20 (3rd, exceeds rem budget if 40+50>80!)
    //  3: "account billing help"       (label 2, tokens 35) -> score 1/5 = 0.20 (4th, fits budget 40+35=75 <= 80!)
    //  4: "shipping status check"      (label 3, tokens 20) -> score 0.00
    const pool: [5]FewShotExample = [
        FewShotExample{ id: 10, input: "reset account password now", output: "auth_reset", label: 1, est_tokens: 40 },
        FewShotExample{ id: 11, input: "reset account email", output: "auth_email", label: 1, est_tokens: 30 },
        FewShotExample{ id: 12, input: "password policy guide", output: "policy_doc", label: 2, est_tokens: 50 },
        FewShotExample{ id: 13, input: "account billing help", output: "billing_faq", label: 2, est_tokens: 35 },
        FewShotExample{ id: 14, input: "shipping status check", output: "shipping", label: 3, est_tokens: 20 },
    ];

    const cfg = FewShotConfig{
        max_examples: 3,
        max_tokens: 80,
        max_per_label: 1,
        dedup_threshold: 0.9,
    };

    let sel_arr: [5]FewShotSelection = undefined;
    let scores_arr: [5]f64 = undefined;
    let order_arr: [5]usize = undefined;
    let ha_arr: [32]u64 = undefined;
    let hb_arr: [32]u64 = undefined;
    let sel: []FewShotSelection = sel_arr[0..];
    let scores: []f64 = scores_arr[0..];
    let order: []usize = order_arr[0..];
    let ha: []u64 = ha_arr[0..];
    let hb: []u64 = hb_arr[0..];

    if (k := selectFewShotExamples("reset account password", pool[0..], cfg, sel, scores, order, ha, hb)) {
        // Selected must be:
        // - idx 0 (label 1, 40 tok, score 0.75)
        // - idx 1 skipped (label 1 already at max_per_label=1)
        // - idx 2 skipped (40 + 50 = 90 > 80)
        // - idx 3 selected (label 2, 35 tok, total 75 <= 80)
        // - idx 4 skipped (75 + 20 = 95 > 80)
        expect_eq(k, 2);
        expect_eq(sel[0].example_index, 0);
        expect_eq(approxEqual(sel[0].score, 0.75, 1.0e-9), true);
        expect_eq(sel[1].example_index, 3);
        expect_eq(approxEqual(sel[1].score, 0.2, 1.0e-9), true);
    } else {
        expect_eq(true, false);
    }
}

fn testDedupThresholdSkipsNearDuplicates()! {
    // Pool has near-duplicate inputs (Jaccard = 1.0 between 0 & 1) and a distinct 3rd exemplar
    const pool: [3]FewShotExample = [
        FewShotExample{ id: 1, input: "cancel my order", output: "cancel_flow", label: 1, est_tokens: 15 },
        FewShotExample{ id: 2, input: "Cancel my order!", output: "cancel_flow_dup", label: 1, est_tokens: 15 },
        FewShotExample{ id: 3, input: "cancel subscription plan", output: "sub_flow", label: 1, est_tokens: 15 },
    ];

    const cfg = FewShotConfig{
        max_examples: 3,
        max_tokens: 200,
        max_per_label: 0,
        dedup_threshold: 0.85,
    };

    let sel_arr: [3]FewShotSelection = undefined;
    let scores_arr: [3]f64 = undefined;
    let order_arr: [3]usize = undefined;
    let ha_arr: [32]u64 = undefined;
    let hb_arr: [32]u64 = undefined;
    let sel: []FewShotSelection = sel_arr[0..];
    let scores: []f64 = scores_arr[0..];
    let order: []usize = order_arr[0..];
    let ha: []u64 = ha_arr[0..];
    let hb: []u64 = hb_arr[0..];

    if (k := selectFewShotExamples("cancel my order", pool[0..], cfg, sel, scores, order, ha, hb)) {
        // idx 0 picked (score 1.0), idx 1 skipped (sim 1.0 > 0.85 with idx 0), idx 2 picked (sim 1/5 = 0.2 <= 0.85)
        expect_eq(k, 2);
        expect_eq(sel[0].example_index, 0);
        expect_eq(sel[1].example_index, 2);
    } else {
        expect_eq(true, false);
    }
}

fn testRenderFewShotBlockExactOutput()! {
    const pool: [3]FewShotExample = [
        FewShotExample{ id: 1, input: "2+2", output: "4", label: 0, est_tokens: 5 },
        FewShotExample{ id: 2, input: "3*5", output: "15", label: 0, est_tokens: 5 },
        FewShotExample{ id: 3, input: "9-4", output: "5", label: 0, est_tokens: 5 },
    ];
    const selected: [2]FewShotSelection = [
        FewShotSelection{ example_index: 2, score: 0.9 },
        FewShotSelection{ example_index: 0, score: 0.5 },
    ];

    let buf_arr: [128]u8 = undefined;
    let buf: []u8 = buf_arr[0..];
    const expected = "Q: 9-4\nA: 5\n---\nQ: 2+2\nA: 4";

    if (n := renderFewShotBlock(pool[0..], selected[0..], "Q: ", "\nA: ", "\n---\n", buf)) {
        expect_eq(n, expected.len);
        expect_eq(bytesEqual(buf[0..n], expected), true);
    } else {
        expect_eq(true, false);
    }
}

fn testDefensiveGuardsEmptyPoolZeroBudgetBuffersAndNamespace()! {
    const empty_pool: []const FewShotExample = [];
    const pool: [2]FewShotExample = [
        FewShotExample{ id: 1, input: "alpha beta", output: "yes", label: 1, est_tokens: 10 },
        FewShotExample{ id: 2, input: "alpha gamma", output: "no", label: 2, est_tokens: 10 },
    ];

    let sel_arr: [4]FewShotSelection = undefined;
    let scores_arr: [4]f64 = undefined;
    let order_arr: [4]usize = undefined;
    let ha_arr: [16]u64 = undefined;
    let hb_arr: [16]u64 = undefined;
    let sel: []FewShotSelection = sel_arr[0..];
    let scores: []f64 = scores_arr[0..];
    let order: []usize = order_arr[0..];
    let ha: []u64 = ha_arr[0..];
    let hb: []u64 = hb_arr[0..];

    const def_cfg = FewShotConfig.default();

    // 1. Empty pool -> 0
    if (k0 := selectFewShotExamples("alpha", empty_pool, def_cfg, sel, scores, order, ha, hb)) {
        expect_eq(k0, 0);
    } else {
        expect_eq(true, false);
    }

    // 2. Zero token budget -> 0
    const zero_budget = FewShotConfig{ max_examples: 5, max_tokens: 0, max_per_label: 0, dedup_threshold: 0.9 };
    if (kz := selectFewShotExamples("alpha", pool[0..], zero_budget, sel, scores, order, ha, hb)) {
        expect_eq(kz, 0);
    } else {
        expect_eq(true, false);
    }

    // 3. Insufficient scratch or output buffers -> null
    let tiny_scores_arr: [1]f64 = undefined;
    let tiny_scores: []f64 = tiny_scores_arr[0..];
    expect_eq(selectFewShotExamples("alpha", pool[0..], def_cfg, sel, tiny_scores, order, ha, hb) == null, true);

    let tiny_sel_arr: [1]FewShotSelection = undefined;
    let tiny_sel: []FewShotSelection = tiny_sel_arr[0..];
    expect_eq(selectFewShotExamples("alpha", pool[0..], def_cfg, tiny_sel, scores, order, ha, hb) == null, true);

    // 4. Out-of-bounds index and tiny buffer in renderFewShotBlock -> null
    const bad_sel: [1]FewShotSelection = [FewShotSelection{ example_index: 99, score: 1.0 }];
    let out_arr: [64]u8 = undefined;
    let out: []u8 = out_arr[0..];
    expect_eq(renderFewShotBlock(pool[0..], bad_sel[0..], "I:", "O:", "\n", out) == null, true);

    let tiny_out_arr: [4]u8 = undefined;
    let tiny_out: []u8 = tiny_out_arr[0..];
    const ok_sel: [1]FewShotSelection = [FewShotSelection{ example_index: 0, score: 1.0 }];
    expect_eq(renderFewShotBlock(pool[0..], ok_sel[0..], "Input: ", " -> ", "\n", tiny_out) == null, true);

    // 5. Namespace access via `cap.prompt.*`
    if (kn := cap.prompt.selectFewShotExamples("alpha beta", pool[0..], def_cfg, sel, scores, order, ha, hb)) {
        expect_eq(kn, 2);
        expect_eq(sel[0].example_index, 0);
    } else {
        expect_eq(true, false);
    }
}

pub fn main()! {
    test("1. prompt/fewshot: tokenJaccardSimilarity exact, partial, orthogonal & case-insensitive", () => {
        testTokenJaccardSimilarityMetrics()?;
    });
    test("2. prompt/fewshot: relevance ranking with max_tokens budget and max_per_label balance", () => {
        testRelevanceBudgetAndClassBalance()?;
    });
    test("3. prompt/fewshot: dedup_threshold skips near-duplicate exemplars", () => {
        testDedupThresholdSkipsNearDuplicates()?;
    });
    test("4. prompt/fewshot: renderFewShotBlock exact prompt block rendering", () => {
        testRenderFewShotBlockExactOutput()?;
    });
    test("5. prompt/fewshot: empty pool, zero budget, buffer guards & cap.prompt namespace", () => {
        testDefensiveGuardsEmptyPoolZeroBudgetBuffersAndNamespace()?;
    });
}
