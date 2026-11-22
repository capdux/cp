// Origin: Pure-CP Stdlib Test Suite (lib/attention/constrained_mask.cp) | Created: 2026-11-22
// Production Deep Verification Suite for LLM structured constrained decoding:
// VocabBitset set/intersect/@popCount & tail-bit hygiene, exact -inf logit masking,
// DFA state-guided zero-retry generation to accepting state, temperature + Top-K masking
// with all-forbidden fallback, and invalid vocab/DFA/temperature defensive guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    VocabBitset,
    DfaEdge,
    ConstrainedDfa,
    applyVocabMaskInPlace,
    applyTemperatureAndTopKMaskInPlace,
} from "cap:attention";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

fn isNegInf(x: f32): bool {
    return std.math.isInf(x) && x < 0.0;
}

fn argmaxF32(logits: []const f32): usize {
    let best_i: usize = 0;
    let best_v: f32 = logits[0];
    let i: usize = 1;
    while (i < logits.len) : (i += 1) {
        if (logits[i] > best_v) {
            best_v = logits[i];
            best_i = i;
        }
    }
    return best_i;
}

// ============================================================================
// Test 1: VocabBitset Set / Intersect / @popCount & Tail-Bit Hygiene
// ============================================================================

fn testVocabBitsetSetIntersectPopcountAndTailHygiene()! {
    expect_eq(VocabBitset.wordsNeeded(0), 0 as usize);
    expect_eq(VocabBitset.wordsNeeded(64), 1 as usize);
    expect_eq(VocabBitset.wordsNeeded(70), 2 as usize);

    let words_a: [2]u64 = [0xFFFF, 0xFFFF];
    let words_a_s: []u64 = words_a[0..];

    if (bs_init := cap.attention.VocabBitset.init(words_a_s, 70)) {
        let bs = bs_init;
        expect_eq(bs.allowedCount(), 0 as usize);

        // allowAll must set exactly 70 bits and strictly zero the 58 unused tail bits in word 1.
        bs.allowAll();
        expect_eq(bs.allowedCount(), 70 as usize);
        expect_eq(bs.isAllowed(0), true);
        expect_eq(bs.isAllowed(69), true);
        expect_eq(bs.isAllowed(70), false);

        expect_eq(bs.forbidToken(5), true);
        expect_eq(bs.forbidToken(69), true);
        expect_eq(bs.forbidToken(70), false);
        expect_eq(bs.allowedCount(), 68 as usize);
        expect_eq(bs.isAllowed(5), false);

        bs.clearAll();
        expect_eq(bs.allowedCount(), 0 as usize);
        expect_eq(bs.allowToken(2), true);
        expect_eq(bs.allowToken(65), true);
        expect_eq(bs.allowToken(68), true);
        expect_eq(bs.allowedCount(), 3 as usize);

        // Intersect with another bitset {2, 65, 69} -> {2, 65}.
        let words_b: [2]u64 = [0, 0];
        let words_b_s: []u64 = words_b[0..];
        if (bs2_init := VocabBitset.init(words_b_s, 70)) {
            let bs2 = bs2_init;
            * = bs2.allowToken(2);
            * = bs2.allowToken(65);
            * = bs2.allowToken(69);
            expect_eq(bs.intersectWith(words_b[0..]), true);
            expect_eq(bs.allowedCount(), 2 as usize);
            expect_eq(bs.isAllowed(2), true);
            expect_eq(bs.isAllowed(65), true);
            expect_eq(bs.isAllowed(68), false);
            expect_eq(bs.isAllowed(69), false);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 2: Logits Vocab Masking Sets Forbidden & NaN Entries Strictly to -inf
// ============================================================================

fn testApplyVocabMaskExactNegInf()! {
    let logits: [8]f32 = [1.0, 5.0, -2.0, 3.0, 0.5, 4.0, std.math.nan(f32), 2.5];
    let logits_s: []f32 = logits[0..];

    // Allow tokens {1, 3, 6, 7}; token 6 is NaN and must be masked to -inf, leaving 3 finite hits.
    let mask_words: [1]u64 = [0];
    let mask_s: []u64 = mask_words[0..];
    if (bs_init := VocabBitset.init(mask_s, 8)) {
        let bs = bs_init;
        * = bs.allowToken(1);
        * = bs.allowToken(3);
        * = bs.allowToken(6);
        * = bs.allowToken(7);

        const kept: usize = cap.attention.applyVocabMaskInPlace(logits_s, mask_words[0..]) ?? 99;
        expect_eq(kept, 3 as usize);
        expect_eq(isNegInf(logits[0]), true);
        expect_eq(absF32(logits[1] - 5.0) < 0.0001, true);
        expect_eq(isNegInf(logits[2]), true);
        expect_eq(absF32(logits[3] - 3.0) < 0.0001, true);
        expect_eq(isNegInf(logits[4]), true);
        expect_eq(isNegInf(logits[5]), true);
        expect_eq(isNegInf(logits[6]), true);
        expect_eq(absF32(logits[7] - 2.5) < 0.0001, true);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 3: DFA State-Guided Zero-Retry Structured Decoding to Accepting State
// ============================================================================

fn testDfaStepByStepConstrainedGenerationToAccepting()! {
    // Mini JSON object DFA:
    // token 0: '{', token 1: '"ok":', token 2: 'true', token 3: 'false', token 4: '}'
    // state 0 --0--> state 1 --1--> state 2 --(2|3)--> state 3 --4--> state 4 (accepting)
    const edges: [5]DfaEdge = [
        DfaEdge{ from_state: 0, to_state: 1, token_id: 0 },
        DfaEdge{ from_state: 1, to_state: 2, token_id: 1 },
        DfaEdge{ from_state: 2, to_state: 3, token_id: 2 },
        DfaEdge{ from_state: 2, to_state: 3, token_id: 3 },
        DfaEdge{ from_state: 3, to_state: 4, token_id: 4 },
    ];
    const accept_bits: [1]u64 = [16]; // bit 4 -> state 4 accepting

    if (dfa := cap.attention.ConstrainedDfa.init(edges[0..], accept_bits[0..], 5, 16)) {
        let state: u16 = 0;
        let generated: [4]u32 = [0; 4];
        let mask_words: [1]u64 = [0];
        let mask_s: []u64 = mask_words[0..];

        let step_idx: usize = 0;
        while (step_idx < 4) : (step_idx += 1) {
            expect_eq(dfa.isAccepting(state), false);

            // Raw LLM logits deliberately give highest score (99.0) to invalid noise token 10,
            // and give token 3 (6.0) higher score than token 2 (2.0) at the boolean branch.
            let logits: [16]f32 = [1.0; 16];
            logits[2] = 2.0;
            logits[3] = 6.0;
            logits[10] = 99.0;
            let logits_s: []f32 = logits[0..];

            const allowed_n: usize = dfa.buildAllowedMask(state, mask_s);
            expect_eq(allowed_n > 0, true);

            const kept: usize = applyVocabMaskInPlace(logits_s, mask_words[0..]) ?? 0;
            expect_eq(kept, allowed_n);
            expect_eq(isNegInf(logits[10]), true);

            const chosen: u32 = argmaxF32(logits[0..]) as u32;
            generated[step_idx] = chosen;
            if (next_state := dfa.step(state, chosen)) {
                state = next_state;
            } else {
                expect_eq(true, false);
            }
        }

        // Exact zero-retry structured output: [0 ('{'), 1 ('"ok":'), 3 ('false'), 4 ('}')]
        expect_eq(generated[0], 0 as u32);
        expect_eq(generated[1], 1 as u32);
        expect_eq(generated[2], 3 as u32);
        expect_eq(generated[3], 4 as u32);
        expect_eq(state, 4 as u16);
        expect_eq(dfa.isAccepting(state), true);
        expect_eq(dfa.step(state, 0) == null, true);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 4: Temperature + Top-K Filtering & All-Forbidden Safe Fallback
// ============================================================================

fn testTemperatureTopKMaskAndAllForbiddenFallback()! {
    let logits: [8]f32 = [1.0, 4.0, 2.0, 4.0, -1.0, 3.0, 0.0, 2.0];
    let logits_s: []f32 = logits[0..];

    // temperature = 2.0, top_k = 3 -> keeps {1 (2.0), 3 (2.0), 5 (1.5)}, masks rest to -inf.
    const kept: usize = cap.attention.applyTemperatureAndTopKMaskInPlace(logits_s, 2.0, 3) ?? 99;
    expect_eq(kept, 3 as usize);
    expect_eq(isNegInf(logits[0]), true);
    expect_eq(absF32(logits[1] - 2.0) < 0.0001, true);
    expect_eq(isNegInf(logits[2]), true);
    expect_eq(absF32(logits[3] - 2.0) < 0.0001, true);
    expect_eq(isNegInf(logits[4]), true);
    expect_eq(absF32(logits[5] - 1.5) < 0.0001, true);
    expect_eq(isNegInf(logits[6]), true);
    expect_eq(isNegInf(logits[7]), true);

    // All-forbidden mask returns 0 cleanly, and downstream Top-K also safely returns 0.
    const zero_mask: [1]u64 = [0];
    expect_eq(applyVocabMaskInPlace(logits_s, zero_mask[0..]) ?? 99, 0 as usize);
    expect_eq(applyTemperatureAndTopKMaskInPlace(logits_s, 1.0, 3) ?? 99, 0 as usize);
}

// ============================================================================
// Test 5: Invalid Vocab, DFA & Temperature Defensive Guards
// ============================================================================

fn testInvalidVocabDfaAndTemperatureGuards()! {
    let w1: [1]u64 = [0];
    let w1_s: []u64 = w1[0..];
    expect_eq(VocabBitset.init(w1_s, 0) == null, true);
    expect_eq(VocabBitset.init(w1_s, 65) == null, true); // needs 2 words

    const bad_state_edge: [1]DfaEdge = [DfaEdge{ from_state: 0, to_state: 5, token_id: 1 }];
    const bad_tok_edge: [1]DfaEdge = [DfaEdge{ from_state: 0, to_state: 1, token_id: 16 }];
    const ok_edge: [1]DfaEdge = [DfaEdge{ from_state: 0, to_state: 1, token_id: 1 }];
    const acc: [1]u64 = [2];
    const empty_acc: []const u64 = acc[0..0];

    expect_eq(ConstrainedDfa.init(ok_edge[0..], acc[0..], 0, 16) == null, true);
    expect_eq(ConstrainedDfa.init(ok_edge[0..], acc[0..], 2, 0) == null, true);
    expect_eq(ConstrainedDfa.init(ok_edge[0..], empty_acc, 2, 16) == null, true);
    expect_eq(ConstrainedDfa.init(bad_state_edge[0..], acc[0..], 2, 16) == null, true);
    expect_eq(ConstrainedDfa.init(bad_tok_edge[0..], acc[0..], 2, 16) == null, true);

    let logits: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    let logits_s: []f32 = logits[0..];
    let empty_logits: []f32 = logits[0..0];
    expect_eq(applyVocabMaskInPlace(empty_logits, w1[0..]) == null, true);
    expect_eq(applyVocabMaskInPlace(logits_s, empty_acc) == null, true);

    expect_eq(applyTemperatureAndTopKMaskInPlace(empty_logits, 1.0, 2) == null, true);
    expect_eq(applyTemperatureAndTopKMaskInPlace(logits_s, 0.0, 2) == null, true);
    expect_eq(applyTemperatureAndTopKMaskInPlace(logits_s, -0.5, 2) == null, true);
    expect_eq(applyTemperatureAndTopKMaskInPlace(logits_s, std.math.nan(f32), 2) == null, true);
    expect_eq(applyTemperatureAndTopKMaskInPlace(logits_s, std.math.inf(f32), 2) == null, true);
}

fn main() {
    test("1. attention.constrained_mask: VocabBitset set/intersect/@popCount & tail hygiene", () => {
        testVocabBitsetSetIntersectPopcountAndTailHygiene()?;
    });
    test("2. attention.constrained_mask: applyVocabMaskInPlace exact -inf masking", () => {
        testApplyVocabMaskExactNegInf()?;
    });
    test("3. attention.constrained_mask: DFA zero-retry structured generation to accepting state", () => {
        testDfaStepByStepConstrainedGenerationToAccepting()?;
    });
    test("4. attention.constrained_mask: temperature + Top-K masking & all-forbidden fallback", () => {
        testTemperatureTopKMaskAndAllForbiddenFallback()?;
    });
    test("5. attention.constrained_mask: invalid vocab, DFA and temperature defensive guards", () => {
        testInvalidVocabDfaAndTemperatureGuards()?;
    });
}
