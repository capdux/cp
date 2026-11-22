// Origin: Pure-CP Stdlib Test Suite (Sparse MoE Top-K Router, Load-Balance Loss & Dispatch) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/moe.cp
// Covers:
//   1. Softmax & Sigmoid Top-2 routing, renorm_topk exact sum == 1.0 & deterministic ascending expert_id tie-break
//   2. DeepSeek-V3 auxiliary-loss-free expert_bias balancing (shifts selection while preserving unbiased gate weights)
//   3. moeDispatchBatch CSR bucketing: sufficient capacity (dropped == 0) vs overload truncation & dropped accounting
//   4. moeLoadBalanceLoss: perfectly balanced routing == 1.0 & total single-expert collapse == E
//   5. moeCombineOutputs exact weighted fusion of multi-expert hidden vectors
//   6. Boundary & malformed input defenses (top_k == 0, top_k > E, undersized buffers, out-of-range IDs, NaN/Inf)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MoeGateKind,
    MoeRouteEntry,
    MoeDispatchStats,
    moeRouteToken,
    moeDispatchBatch,
    moeLoadBalanceLoss,
    moeCombineOutputs,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testSoftmaxAndSigmoidTopKRouting()! {
    const logits: [4]f64 = [0.5, 2.0, -1.0, 1.5];
    const no_bias: []const f64 = logits[0..0];

    // 1a. Softmax Top-2 with renorm_topk = true -> experts [1, 3], weights sum to 1.0
    let routes_buf: [2]MoeRouteEntry = [MoeRouteEntry{}; 2];
    let routes: []MoeRouteEntry = routes_buf[0..];
    expect_eq(moeRouteToken(logits[0..], no_bias, 2, MoeGateKind.Softmax, true, routes), true);
    expect_eq(routes_buf[0].expert_id, 1);
    expect_eq(routes_buf[1].expert_id, 3);
    expect_eq(approxF64(routes_buf[0].weight + routes_buf[1].weight, 1.0, EPS_F64), true);

    const e1 = std.math.exp(2.0 - 2.0);
    const e3 = std.math.exp(1.5 - 2.0);
    expect_eq(approxF64(routes_buf[0].weight, e1 / (e1 + e3), EPS_F64), true);
    expect_eq(approxF64(routes_buf[1].weight, e3 / (e1 + e3), EPS_F64), true);

    // 1b. Softmax Top-2 with renorm_topk = false -> raw full-softmax probabilities
    expect_eq(moeRouteToken(logits[0..], no_bias, 2, MoeGateKind.Softmax, false, routes), true);
    const full_sum = std.math.exp(0.5 - 2.0) + 1.0 + std.math.exp(-1.0 - 2.0) + std.math.exp(1.5 - 2.0);
    expect_eq(approxF64(routes_buf[0].weight, 1.0 / full_sum, EPS_F64), true);
    expect_eq(approxF64(routes_buf[1].weight, std.math.exp(1.5 - 2.0) / full_sum, EPS_F64), true);

    // 1c. Sigmoid Top-2 (DeepSeek-V3 style) with renorm_topk = true & false
    expect_eq(moeRouteToken(logits[0..], no_bias, 2, MoeGateKind.Sigmoid, false, routes), true);
    const s1 = 1.0 / (1.0 + std.math.exp(-2.0));
    const s3 = 1.0 / (1.0 + std.math.exp(-1.5));
    expect_eq(routes_buf[0].expert_id, 1);
    expect_eq(routes_buf[1].expert_id, 3);
    expect_eq(approxF64(routes_buf[0].weight, s1, EPS_F64), true);
    expect_eq(approxF64(routes_buf[1].weight, s3, EPS_F64), true);

    expect_eq(moeRouteToken(logits[0..], no_bias, 2, MoeGateKind.Sigmoid, true, routes), true);
    expect_eq(approxF64(routes_buf[0].weight + routes_buf[1].weight, 1.0, EPS_F64), true);
    expect_eq(approxF64(routes_buf[0].weight, s1 / (s1 + s3), EPS_F64), true);
    expect_eq(approxF64(routes_buf[1].weight, s3 / (s1 + s3), EPS_F64), true);

    // 1d. Deterministic tie-breaking by ascending expert_id
    const tied_logits: [4]f64 = [1.0, 1.0, 1.0, 1.0];
    expect_eq(moeRouteToken(tied_logits[0..], no_bias, 2, MoeGateKind.Softmax, true, routes), true);
    expect_eq(routes_buf[0].expert_id, 0);
    expect_eq(routes_buf[1].expert_id, 1);
    expect_eq(approxF64(routes_buf[0].weight, 0.5, EPS_F64), true);
    expect_eq(approxF64(routes_buf[1].weight, 0.5, EPS_F64), true);
}

fn testExpertBiasAuxiliaryFreeBalancing()! {
    const logits: [4]f64 = [1.2, 1.0, 0.4, 0.2];
    const no_bias: []const f64 = logits[0..0];

    let routes_buf: [2]MoeRouteEntry = [MoeRouteEntry{}; 2];
    let routes: []MoeRouteEntry = routes_buf[0..];

    // Without bias, Top-2 selects experts 0 and 1.
    expect_eq(moeRouteToken(logits[0..], no_bias, 2, MoeGateKind.Sigmoid, true, routes), true);
    expect_eq(routes_buf[0].expert_id, 0);
    expect_eq(routes_buf[1].expert_id, 1);

    // Apply DeepSeek-V3 balancing bias: penalize overloaded experts 0,1 and boost 2,3.
    const bias: [4]f64 = [-0.5, -0.5, 0.4, 0.4];
    expect_eq(moeRouteToken(logits[0..], bias[0..], 2, MoeGateKind.Sigmoid, true, routes), true);
    expect_eq(routes_buf[0].expert_id, 2);
    expect_eq(routes_buf[1].expert_id, 3);
    expect_eq(approxF64(routes_buf[0].weight + routes_buf[1].weight, 1.0, EPS_F64), true);

    // Emitted weights must reflect unbiased sigmoid scores of experts 2 & 3, renormalized.
    const s2 = 1.0 / (1.0 + std.math.exp(-0.4));
    const s3 = 1.0 / (1.0 + std.math.exp(-0.2));
    expect_eq(approxF64(routes_buf[0].weight, s2 / (s2 + s3), EPS_F64), true);
    expect_eq(approxF64(routes_buf[1].weight, s3 / (s2 + s3), EPS_F64), true);
}

fn testDispatchBatchCapacityAndDropAccounting()! {
    // 4 tokens, top_k = 2, n_experts = 3 -> 8 total assignments:
    // Expert 0 gets tokens [0, 1, 2, 3] (hot expert!)
    // Expert 1 gets tokens [0, 2]
    // Expert 2 gets tokens [1, 3]
    const routes: [8]MoeRouteEntry = [
        MoeRouteEntry{ expert_id: 0, weight: 0.7 },
        MoeRouteEntry{ expert_id: 1, weight: 0.3 },
        MoeRouteEntry{ expert_id: 0, weight: 0.6 },
        MoeRouteEntry{ expert_id: 2, weight: 0.4 },
        MoeRouteEntry{ expert_id: 0, weight: 0.8 },
        MoeRouteEntry{ expert_id: 1, weight: 0.2 },
        MoeRouteEntry{ expert_id: 0, weight: 0.55 },
        MoeRouteEntry{ expert_id: 2, weight: 0.45 },
    ];

    let offsets_buf: [4]usize = [0; 4];
    let tokens_buf: [8]usize = [0; 8];
    let weights_buf: [8]f64 = [0.0; 8];
    let scratch_buf: [3]usize = [0; 3];
    let offsets: []usize = offsets_buf[0..];
    let tokens: []usize = tokens_buf[0..];
    let weights: []f64 = weights_buf[0..];
    let scratch: []usize = scratch_buf[0..];

    // Case A: sufficient capacity (expert_capacity = 4) -> dropped == 0
    if (st_full := moeDispatchBatch(routes[0..], 4, 2, 3, 4, offsets, tokens, weights, scratch)) {
        expect_eq(st_full.total_assignments, 8);
        expect_eq(st_full.dispatched, 8);
        expect_eq(st_full.dropped, 0);
        // CSR offsets: [0, 4, 6, 8]
        expect_eq(offsets_buf[0], 0);
        expect_eq(offsets_buf[1], 4);
        expect_eq(offsets_buf[2], 6);
        expect_eq(offsets_buf[3], 8);
        // Expert 0 bucket: tokens 0, 1, 2, 3
        expect_eq(tokens_buf[0], 0);
        expect_eq(tokens_buf[1], 1);
        expect_eq(tokens_buf[2], 2);
        expect_eq(tokens_buf[3], 3);
        expect_eq(approxF64(weights_buf[2], 0.8, EPS_F64), true);
        // Expert 1 bucket: tokens 0, 2
        expect_eq(tokens_buf[4], 0);
        expect_eq(tokens_buf[5], 2);
        // Expert 2 bucket: tokens 1, 3
        expect_eq(tokens_buf[6], 1);
        expect_eq(tokens_buf[7], 3);
    } else {
        expect_eq(true, false);
    }

    // Case B: capacity-constrained (expert_capacity = 2) -> expert 0 drops 2 tokens
    if (st_cap := moeDispatchBatch(routes[0..], 4, 2, 3, 2, offsets, tokens, weights, scratch)) {
        expect_eq(st_cap.total_assignments, 8);
        expect_eq(st_cap.dispatched, 6);
        expect_eq(st_cap.dropped, 2);
        // CSR offsets: [0, 2, 4, 6]
        expect_eq(offsets_buf[0], 0);
        expect_eq(offsets_buf[1], 2);
        expect_eq(offsets_buf[2], 4);
        expect_eq(offsets_buf[3], 6);
        // Expert 0 keeps first 2 arriving tokens: 0 and 1
        expect_eq(tokens_buf[0], 0);
        expect_eq(tokens_buf[1], 1);
        expect_eq(approxF64(weights_buf[0], 0.7, EPS_F64), true);
        expect_eq(approxF64(weights_buf[1], 0.6, EPS_F64), true);
        // Expert 1 & 2 unaffected
        expect_eq(tokens_buf[2], 0);
        expect_eq(tokens_buf[3], 2);
        expect_eq(tokens_buf[4], 1);
        expect_eq(tokens_buf[5], 3);
    } else {
        expect_eq(true, false);
    }

    // Case C: expert_capacity == 0 -> all dropped
    if (st_zero := moeDispatchBatch(routes[0..], 4, 2, 3, 0, offsets, tokens, weights, scratch)) {
        expect_eq(st_zero.total_assignments, 8);
        expect_eq(st_zero.dispatched, 0);
        expect_eq(st_zero.dropped, 8);
        expect_eq(offsets_buf[3], 0);
    } else {
        expect_eq(true, false);
    }

    // Case D: n_tokens == 0 -> empty dispatch succeeds
    if (st_empty := moeDispatchBatch(routes[0..0], 0, 2, 3, 2, offsets, tokens, weights, scratch)) {
        expect_eq(st_empty.total_assignments, 0);
        expect_eq(st_empty.dispatched, 0);
        expect_eq(st_empty.dropped, 0);
        expect_eq(offsets_buf[0], 0);
        expect_eq(offsets_buf[3], 0);
    } else {
        expect_eq(true, false);
    }
}

fn testLoadBalanceLossBalancedAndCollapsed()! {
    let frac_buf: [4]f64 = [0.0; 4];
    let prob_buf: [4]f64 = [0.0; 4];
    let frac: []f64 = frac_buf[0..];
    let prob: []f64 = prob_buf[0..];

    // 4a. Perfectly balanced routing (E = 4, N = 4, top_k = 2) -> L_aux == 1.0
    const balanced_probs: [16]f64 = [
        0.25, 0.25, 0.25, 0.25,
        0.25, 0.25, 0.25, 0.25,
        0.25, 0.25, 0.25, 0.25,
        0.25, 0.25, 0.25, 0.25,
    ];
    const balanced_chosen: [8]usize = [0, 1, 2, 3, 1, 2, 3, 0];
    if (loss_bal := moeLoadBalanceLoss(balanced_probs[0..], balanced_chosen[0..], 4, 4, 2, frac, prob)) {
        expect_eq(approxF64(loss_bal, 1.0, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // 4b. Total collapse onto a single expert (E = 4, N = 4, top_k = 1) -> L_aux == E == 4.0
    const collapsed_probs: [16]f64 = [
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
    ];
    const collapsed_chosen: [4]usize = [2, 2, 2, 2];
    if (loss_col := moeLoadBalanceLoss(collapsed_probs[0..], collapsed_chosen[0..], 4, 4, 1, frac, prob)) {
        expect_eq(approxF64(loss_col, 4.0, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // 4c. Partial skew (E = 4, N = 2, top_k = 1):
    // expert 0 gets 100% of tokens (f_0 = 1.0), mean probs P = [0.7, 0.1, 0.1, 0.1]
    // L_aux = 4 * (1.0 * 0.7) = 2.8
    const skewed_probs: [8]f64 = [
        0.8, 0.1, 0.05, 0.05,
        0.6, 0.1, 0.15, 0.15,
    ];
    const skewed_chosen: [2]usize = [0, 0];
    if (loss_skew := moeLoadBalanceLoss(skewed_probs[0..], skewed_chosen[0..], 2, 4, 1, frac, prob)) {
        expect_eq(approxF64(loss_skew, 2.8, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }
}

fn testCombineOutputsExactWeightedSum()! {
    const logits: [4]f64 = [0.0, 2.0, 1.0, -1.0];
    const no_bias: []const f64 = logits[0..0];
    let routes_buf: [2]MoeRouteEntry = [MoeRouteEntry{}; 2];
    let routes: []MoeRouteEntry = routes_buf[0..];
    expect_eq(moeRouteToken(logits[0..], no_bias, 2, MoeGateKind.Softmax, true, routes), true);

    // Two selected expert outputs of hidden_dim = 4
    const expert_outs: [8]f64 = [
        1.0, -2.0, 3.5, 4.0,
        5.0,  6.0, -1.5, 2.0,
    ];
    let out_buf: [4]f64 = [99.0; 4]; // pre-filled to confirm zero-initialization
    let out: []f64 = out_buf[0..];

    expect_eq(moeCombineOutputs(expert_outs[0..], routes_buf[0..], 4, out), true);

    const w0 = routes_buf[0].weight;
    const w1 = routes_buf[1].weight;
    expect_eq(approxF64(out_buf[0], w0 * 1.0 + w1 * 5.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[1], w0 * (-2.0) + w1 * 6.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[2], w0 * 3.5 + w1 * (-1.5), EPS_F64), true);
    expect_eq(approxF64(out_buf[3], w0 * 4.0 + w1 * 2.0, EPS_F64), true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const logits: [4]f64 = [0.1, 0.2, 0.3, 0.4];
    const no_bias: []const f64 = logits[0..0];
    const short_bias: [2]f64 = [0.1, 0.2];
    const nan_val = std.math.nan(f64);
    const inf_val = std.math.inf(f64);

    let routes_buf: [4]MoeRouteEntry = [MoeRouteEntry{}; 4];
    let routes: []MoeRouteEntry = routes_buf[0..];
    let short_routes: []MoeRouteEntry = routes_buf[0..1];

    // moeRouteToken defenses
    expect_eq(moeRouteToken(logits[0..0], no_bias, 1, MoeGateKind.Softmax, true, routes), false); // empty logits
    expect_eq(moeRouteToken(logits[0..], no_bias, 0, MoeGateKind.Softmax, true, routes), false);  // top_k == 0
    expect_eq(moeRouteToken(logits[0..], no_bias, 5, MoeGateKind.Softmax, true, routes), false);  // top_k > E
    expect_eq(moeRouteToken(logits[0..], no_bias, 2, MoeGateKind.Softmax, true, short_routes), false); // undersized out
    expect_eq(moeRouteToken(logits[0..], short_bias[0..], 2, MoeGateKind.Sigmoid, true, routes), false); // short bias

    const nan_logits: [4]f64 = [0.1, nan_val, 0.3, 0.4];
    const inf_logits: [4]f64 = [0.1, inf_val, 0.3, 0.4];
    const nan_bias: [4]f64 = [0.0, 0.0, nan_val, 0.0];
    expect_eq(moeRouteToken(nan_logits[0..], no_bias, 2, MoeGateKind.Softmax, true, routes), false);
    expect_eq(moeRouteToken(inf_logits[0..], no_bias, 2, MoeGateKind.Sigmoid, true, routes), false);
    expect_eq(moeRouteToken(logits[0..], nan_bias[0..], 2, MoeGateKind.Sigmoid, true, routes), false);

    // moeDispatchBatch defenses
    const valid_routes: [2]MoeRouteEntry = [
        MoeRouteEntry{ expert_id: 0, weight: 0.6 },
        MoeRouteEntry{ expert_id: 1, weight: 0.4 },
    ];
    const oob_routes: [2]MoeRouteEntry = [
        MoeRouteEntry{ expert_id: 0, weight: 0.6 },
        MoeRouteEntry{ expert_id: 4, weight: 0.4 },
    ];
    const nan_routes: [2]MoeRouteEntry = [
        MoeRouteEntry{ expert_id: 0, weight: nan_val },
        MoeRouteEntry{ expert_id: 1, weight: 0.4 },
    ];
    let offsets_buf: [5]usize = [0; 5];
    let tokens_buf: [2]usize = [0; 2];
    let weights_buf: [2]f64 = [0.0; 2];
    let scratch_buf: [4]usize = [0; 4];
    let offsets: []usize = offsets_buf[0..];
    let short_offsets: []usize = offsets_buf[0..3];
    let tokens: []usize = tokens_buf[0..];
    let short_tokens: []usize = tokens_buf[0..1];
    let weights: []f64 = weights_buf[0..];
    let scratch: []usize = scratch_buf[0..];

    expect_eq(moeDispatchBatch(valid_routes[0..], 1, 0, 4, 2, offsets, tokens, weights, scratch) == null, true);
    expect_eq(moeDispatchBatch(valid_routes[0..], 1, 5, 4, 2, offsets, tokens, weights, scratch) == null, true);
    expect_eq(moeDispatchBatch(valid_routes[0..], 1, 2, 4, 2, short_offsets, tokens, weights, scratch) == null, true);
    expect_eq(moeDispatchBatch(valid_routes[0..], 1, 2, 4, 2, offsets, short_tokens, weights, scratch) == null, true);
    expect_eq(moeDispatchBatch(oob_routes[0..], 1, 2, 4, 2, offsets, tokens, weights, scratch) == null, true);
    expect_eq(moeDispatchBatch(nan_routes[0..], 1, 2, 4, 2, offsets, tokens, weights, scratch) == null, true);

    // moeLoadBalanceLoss defenses
    let frac_buf: [4]f64 = [0.0; 4];
    let prob_buf: [4]f64 = [0.0; 4];
    let frac: []f64 = frac_buf[0..];
    let short_frac: []f64 = frac_buf[0..2];
    let prob: []f64 = prob_buf[0..];
    const probs_ok: [4]f64 = [0.25, 0.25, 0.25, 0.25];
    const probs_nan: [4]f64 = [0.25, nan_val, 0.25, 0.25];
    const chosen_ok: [2]usize = [0, 1];
    const chosen_oob: [2]usize = [0, 4];

    expect_eq(moeLoadBalanceLoss(probs_ok[0..], chosen_ok[0..], 0, 4, 2, frac, prob) == null, true);
    expect_eq(moeLoadBalanceLoss(probs_ok[0..], chosen_ok[0..], 1, 4, 0, frac, prob) == null, true);
    expect_eq(moeLoadBalanceLoss(probs_ok[0..], chosen_ok[0..], 1, 4, 5, frac, prob) == null, true);
    expect_eq(moeLoadBalanceLoss(probs_ok[0..], chosen_ok[0..], 1, 4, 2, short_frac, prob) == null, true);
    expect_eq(moeLoadBalanceLoss(probs_nan[0..], chosen_ok[0..], 1, 4, 2, frac, prob) == null, true);
    expect_eq(moeLoadBalanceLoss(probs_ok[0..], chosen_oob[0..], 1, 4, 2, frac, prob) == null, true);

    // moeCombineOutputs defenses
    const expert_outs: [8]f64 = [1.0; 8];
    const expert_outs_nan: [8]f64 = [1.0, nan_val, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0];
    let out_buf: [4]f64 = [0.0; 4];
    let out: []f64 = out_buf[0..];
    let short_out: []f64 = out_buf[0..2];

    expect_eq(moeCombineOutputs(expert_outs[0..], valid_routes[0..0], 4, out), false);
    expect_eq(moeCombineOutputs(expert_outs[0..], valid_routes[0..], 0, out), false);
    expect_eq(moeCombineOutputs(expert_outs[0..4], valid_routes[0..], 4, out), false);
    expect_eq(moeCombineOutputs(expert_outs[0..], valid_routes[0..], 4, short_out), false);
    expect_eq(moeCombineOutputs(expert_outs_nan[0..], valid_routes[0..], 4, out), false);
    expect_eq(moeCombineOutputs(expert_outs[0..], nan_routes[0..], 4, out), false);
}

fn main() {
    test("1. moe: Softmax & Sigmoid Top-K routing + tie-breaking", () => {
        testSoftmaxAndSigmoidTopKRouting()?;
    });
    test("2. moe: DeepSeek-V3 auxiliary-loss-free expert_bias balancing", () => {
        testExpertBiasAuxiliaryFreeBalancing()?;
    });
    test("3. moe: batch CSR dispatch capacity truncation & drop stats", () => {
        testDispatchBatchCapacityAndDropAccounting()?;
    });
    test("4. moe: load-balance auxiliary loss balanced=1.0 & collapsed=E", () => {
        testLoadBalanceLossBalancedAndCollapsed()?;
    });
    test("5. moe: weighted multi-expert output combination", () => {
        testCombineOutputsExactWeightedSum()?;
    });
    test("6. moe: boundary & malformed input defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
